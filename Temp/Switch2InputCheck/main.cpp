#include <cstdio>
#include <exception>
void RunInputTests();
/** @brief 入力テストを実行する @return 成功時0、失敗時1 */
int main() {
    try { RunInputTests(); std::puts("Input tests passed"); return 0; }
    catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
