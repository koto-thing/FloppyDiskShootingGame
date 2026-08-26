#include "../Infrastructure/Services/CollisionService.h"
#include "../Domain/Interfaces/ICollisionReceiver.h"

#include <stdexcept>

namespace {
class Receiver final : public Component, public ICollisionReceiver {
public:
    void OnCollisionEnter(Collider&, Collider&) override { ++enterCount; }
    void OnCollisionStay(Collider&, Collider&) override { ++stayCount; }
    void OnCollisionExit(Collider&, Collider&) override { ++exitCount; }

    int enterCount = 0;
    int stayCount = 0;
    int exitCount = 0;
};

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

void RunCollisionServiceTests() {
    GameObject firstObject;
    GameObject secondObject;
    auto firstReceiver = firstObject.AddComponent<Receiver>();
    auto secondReceiver = secondObject.AddComponent<Receiver>();
    auto firstCollider = firstObject.AddComponent<CircleCollider>(2.0f);
    auto secondCollider = secondObject.AddComponent<CircleCollider>(2.0f);

    firstObject.SetPosition({0.0f, 0.0f, 0.0f});
    secondObject.SetPosition({3.0f, 0.0f, 0.0f});
    firstCollider->SetLayer(CollisionLayer::PLAYER);
    secondCollider->SetLayer(CollisionLayer::ENEMY);
    firstCollider->SetCollisionMask(static_cast<std::uint32_t>(CollisionLayer::ENEMY));
    secondCollider->SetCollisionMask(static_cast<std::uint32_t>(CollisionLayer::PLAYER));

    CollisionService service;
    service.RegisterCollider(firstCollider);
    service.RegisterCollider(secondCollider);
    service.RegisterCollider(firstCollider);
    service.Tick();
    Require(firstReceiver->enterCount == 1 && secondReceiver->enterCount == 1,
            "Collision enter must be dispatched once");

    service.Tick();
    Require(firstReceiver->stayCount == 1 && secondReceiver->stayCount == 1,
            "Collision stay must be dispatched while contact continues");

    secondObject.SetPosition({10.0f, 0.0f, 0.0f});
    service.Tick();
    Require(firstReceiver->exitCount == 1 && secondReceiver->exitCount == 1,
            "Collision exit must be dispatched when contact ends");
}
