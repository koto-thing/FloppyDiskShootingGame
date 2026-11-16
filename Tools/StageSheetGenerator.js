// 敵出現シートと行動スクリプトのJSONからC++ヘッダを生成する
// ブラウザでは Tools/StageEditor.html から読み込み、Nodeでは単体のCLIとして動く
//   node Tools/StageSheetGenerator.js          JSONと.hが一致しているか検査する
//   node Tools/StageSheetGenerator.js --write  JSONから.hを再生成する
(function (exports) {
    "use strict";

    /** @brief 雑魚敵として配置できる敵構成種別（Stage::EnemyTypeの名前） */
    const EnemyTypes = [
        "BasicEnemy", "HeavyEnemy", "StraightShooterEnemy", "ArmoredEnemy",
        "CircleShooterEnemy", "SquareShooterEnemy", "DiveRusherEnemy",
        "MissileShooterEnemy", "LinkedLaserEnemy", "WallSecurityDroneEnemy", "ScriptedEnemy"
    ];
    const Difficulties = ["Easy", "Normal", "Hard"];

    /**
     * @brief JSONのコマンド名とC++のEnemyScript::Opの対応
     *
     * argsはC++側の引数a, b, cへ順に入れる値で、文字列はJSONのキー、数値は固定値を表す
     * WaitとFireAimedは既存のOpの組み合わせで表し、ゲーム側のコードを増やさない
     */
    const CommandSpecs = {
        Wait: { op: "Move", frames: true, args: [0, 0] },
        Move: { op: "Move", frames: true, args: ["vx", "vy"] },
        MoveTo: { op: "MoveTo", frames: true, args: ["x", "y"] },
        Aim: { op: "Aim", frames: true, args: ["speed"] },
        Sine: { op: "Sine", frames: true, args: ["vx", "amplitude", "period"] },
        FireRing: { op: "FireRing", frames: false, args: ["count", "speed", "angle"] },
        FireFan: { op: "FireFan", frames: false, args: ["count", "spread", "speed"] },
        FireAimed: { op: "FireFan", frames: false, args: [1, 0, "speed"] },
        Repeat: { op: "Repeat", frames: false, args: ["target", "count"] }
    };

    /**
     * @brief 数値をC++のfloatリテラルへ変換する
     * @param value 変換する数値
     * @return 末尾にfを付けたリテラル文字列
     */
    function FloatLiteral(value) {
        const text = String(value);
        return (/[.e]/.test(text) ? text : text + ".0") + "f";
    }

    /**
     * @brief JSONのコマンドをC++のOpと引数a, b, cへ変換する
     * @param command JSONのコマンド
     * @return { op, frames, a, b, c }
     */
    function CompileCommand(command) {
        const spec = CommandSpecs[command.op];
        const [a, b, c] = [0, 1, 2].map((i) => {
            const arg = spec.args[i];
            return arg === undefined ? 0 : (typeof arg === "string" ? command[arg] : arg);
        });
        return { op: spec.op, frames: spec.frames ? command.frames : 0, a, b, c };
    }

    /**
     * @brief JSONのコマンドをC++のCommand初期化子1行へ変換する
     * @param command JSONのコマンド
     * @return 出力する1行
     */
    function CommandLine(command) {
        const c = CompileCommand(command);
        return `        {${c.op}, ${c.frames}, ${FloatLiteral(c.a)}, ${FloatLiteral(c.b)}, ${FloatLiteral(c.c)}}, // ${command.op}`;
    }

    /**
     * @brief ScriptedEnemyBehavior::Step と同じ規則で2Dの行動スクリプトを実行する
     *
     * C++側を変えた場合はここも合わせ、EnemyScriptTests の一致検査で差を検出する
     * @param script 行動スクリプト（JSON）
     * @param startX 出現X
     * @param startY 出現Y
     * @param playerX 自機X（狙う方向に使う）
     * @param playerY 自機Y
     * @param maxFrames 実行する最大フレーム数
     * @return { path: 各フレーム後の{x, y, pc}, shots: {frame, pc, x, y, vx, vy} }（pcはその時のコマンド番号）
     */
    function SimulateScript(script, startX, startY, playerX, playerY, maxFrames) {
        const commands = script.commands.map(CompileCommand);
        const e = { x: startX, y: startY, pc: 0, age: 0, loop: 0, vx: 0, vy: 0, baseY: 0 };
        const path = [];
        const shots = [];
        const toRadians = (degrees) => degrees * Math.PI / 180;
        let frame = 0;

        // 等間隔の角度で弾を撃つ
        const fire = (count, start, step, speed) => {
            for (let i = 0; i < count; ++i) {
                const r = toRadians(start + step * i);
                shots.push({ frame, pc: e.pc, x: e.x, y: e.y, vx: Math.cos(r) * speed, vy: Math.sin(r) * speed });
            }
        };

        // コマンド開始時の処理を行い、時間を使わずに進んだ場合trueを返す
        const begin = (command) => {
            const frames = Math.max(1, command.frames);
            const aim = command.op === "Aim" || command.op === "FireFan" ?
                Math.atan2(playerY - e.y, playerX - e.x) * 180 / Math.PI : 0;
            switch (command.op) {
            case "Move": e.vx = command.a; e.vy = command.b; return false;
            case "MoveTo": e.vx = (command.a - e.x) / frames; e.vy = (command.b - e.y) / frames; return false;
            case "Aim": e.vx = Math.cos(toRadians(aim)) * command.a; e.vy = Math.sin(toRadians(aim)) * command.a; return false;
            case "Sine": e.vx = command.a; e.vy = 0; e.baseY = e.y; return false;
            case "FireRing": fire(Math.trunc(command.a), command.c, 360 / Math.max(1, Math.trunc(command.a)), command.b); break;
            case "FireFan": {
                const count = Math.trunc(command.a);
                fire(count, aim - command.b * 0.5, count > 1 ? command.b / (count - 1) : 0, command.c);
                break;
            }
            case "Repeat":
                if (command.b <= 0 || e.loop < Math.trunc(command.b)) { ++e.loop; e.pc = Math.trunc(command.a); }
                else { e.loop = 0; ++e.pc; }
                e.age = 0;
                return true;
            }
            ++e.pc;
            e.age = 0;
            return true;
        };

        // 画面左端を越えて消えるまでフレームを進める
        for (; frame < maxFrames; ++frame) {
            for (let executed = 0; e.pc < commands.length && executed < 16; ++executed) {
                const command = commands[e.pc];
                if (e.age === 0 && begin(command)) continue;
                if (e.age < command.frames) { ++e.age; break; }
                ++e.pc;
                e.age = 0;
            }
            const current = e.pc < commands.length && e.age > 0 ? commands[e.pc] : null;
            e.x += e.vx;
            if (current && current.op === "Sine") e.y = e.baseY + Math.sin(e.age * 2 * Math.PI / current.c) * current.b;
            else e.y += e.vy;
            if (e.pc >= commands.length && e.vx === 0 && e.vy === 0) e.vx = -0.010;
            path.push({ x: e.x, y: e.y, pc: e.pc });
            if (e.x < -2.6) break;
        }
        return { path, shots };
    }

    /** @brief JSとC++の実行結果の一致検査に使う、全コマンドを含むスクリプト */
    const ParityScript = {
        name: "Parity", hp: 1, score: 0, commands: [
            { op: "MoveTo", frames: 30, x: 1.5, y: 0.6 },
            { op: "FireFan", count: 5, spread: 60, speed: 0.02 },
            { op: "Wait", frames: 10 },
            { op: "Repeat", target: 1, count: 1 },
            { op: "Aim", frames: 20, speed: 0.015 },
            { op: "Sine", frames: 60, vx: -0.01, amplitude: 0.25, period: 30 },
            { op: "FireRing", count: 6, speed: 0.01, angle: 15 },
            { op: "FireAimed", speed: 0.03 },
            { op: "Move", frames: 0, vx: -0.03, vy: 0.002 }
        ]
    };

    /**
     * @brief ParityScriptのJS実行結果をC++テスト用ヘッダとして出力する
     * @return ヘッダファイルの内容
     */
    function GenerateParity() {
        const start = [2.8, -0.3, -0.72, 0.0];
        const result = SimulateScript(ParityScript, ...start, 1000);
        const f = (v) => FloatLiteral(Number(v.toPrecision(7)));
        return `#pragma once

// 自動生成ファイル: node Tools/StageSheetGenerator.js --write で再生成する
// エディターのプレビュー(SimulateScript)の結果で、EnemyScriptTests がC++の実行結果と一致するか検証する

namespace EnemyScript::Parity {
    inline constexpr Command Commands[] = {
${ParityScript.commands.map(CommandLine).join("\n")}
    };
    inline constexpr int CommandCount = ${ParityScript.commands.length};
    inline constexpr float StartX = ${f(start[0])}, StartY = ${f(start[1])}, PlayerX = ${f(start[2])}, PlayerY = ${f(start[3])};
    /** @brief 各フレーム実行後の敵座標 { x, y } */
    inline constexpr float Path[][2] = {
${result.path.map((p) => `        {${f(p.x)}, ${f(p.y)}},`).join("\n")}
    };
    /** @brief 発射順の弾 { x, y, vx, vy } */
    inline constexpr float Shots[][4] = {
${result.shots.map((s) => `        {${f(s.x)}, ${f(s.y)}, ${f(s.vx)}, ${f(s.vy)}},`).join("\n")}
    };
}
`;
    }

    /**
     * @brief 行動スクリプト一覧を検証する
     * @param scripts EnemyScripts.jsonの内容
     * @return なし（不正な場合は例外を投げる）
     */
    function ValidateScripts(scripts) {
        const fail = (message) => { throw new Error(`EnemyScripts: ${message}`); };
        if (!Array.isArray(scripts.scripts) || scripts.scripts.length === 0) fail("scriptsが空");
        if (scripts.scripts.length > 255) fail("スクリプトは255個まで");
        const names = new Set();
        scripts.scripts.forEach((script) => {
            // スクリプト全体の値を検証する
            if (!/^[A-Za-z_]\w*$/.test(script.name || "")) fail(`名前 ${script.name} は英数字と_のみ`);
            if (names.has(script.name)) fail(`名前 ${script.name} が重複`);
            names.add(script.name);
            if (!Number.isInteger(script.hp) || script.hp < 1) fail(`${script.name}: hpが不正`);
            if (!Number.isInteger(script.score) || script.score < 0) fail(`${script.name}: scoreが不正`);

            // コマンドごとの値を検証する
            script.commands.forEach((command, i) => {
                const at = `${script.name}のコマンド${i}`;
                const spec = CommandSpecs[command.op];
                if (!spec) fail(`${at}: ${command.op} は未対応`);
                if (spec.frames && (!Number.isInteger(command.frames) || command.frames < 0 || command.frames > 65535)) {
                    fail(`${at}: framesは0から65535の整数`);
                }
                for (const key of spec.args.filter((a) => typeof a === "string")) {
                    if (!Number.isFinite(command[key])) fail(`${at}: ${key}が不正`);
                }
                if (command.op === "MoveTo" && command.frames < 1) fail(`${at}: MoveToのframesは1以上`);
                if (command.op === "Sine" && command.period <= 0) fail(`${at}: periodは正の値`);
                if (spec.args.includes("count") && command.op !== "Repeat" &&
                    (!Number.isInteger(command.count) || command.count < 1 || command.count > 64)) {
                    fail(`${at}: countは1から64の整数`);
                }
                if (command.op === "Repeat") {
                    if (!Number.isInteger(command.target) || command.target < 0 || command.target >= i) {
                        fail(`${at}: targetは自分より前のコマンド番号`);
                    }
                    if (!Number.isInteger(command.count) || command.count < 0) fail(`${at}: countは0以上の整数（0は無限）`);
                    // ゲーム側のループ回数は1つしかないためRepeatの入れ子を禁止する
                    if (script.commands.slice(command.target, i).some((c) => c.op === "Repeat")) {
                        fail(`${at}: Repeatの範囲に別のRepeatがある（入れ子は未対応）`);
                    }
                }
            });
        });
    }

    /**
     * @brief 行動スクリプト一覧からC++ヘッダの文字列を生成する
     * @param scripts EnemyScripts.jsonの内容
     * @return ヘッダファイルの内容
     */
    function GenerateScripts(scripts) {
        ValidateScripts(scripts);
        const lines = [];
        const definitions = [];

        // スクリプトごとのコマンド配列を出力する
        scripts.scripts.forEach((script) => {
            if (script.commands.length === 0) {
                definitions.push(`        {nullptr, 0, ${script.hp}, ${script.score}},`);
                return;
            }
            lines.push(`    inline constexpr Command ${script.name}Commands[] = {`);
            lines.push(...script.commands.map(CommandLine));
            lines.push("    };");
            definitions.push(`        {${script.name}Commands, ${script.commands.length}, ${script.hp}, ${script.score}},`);
        });

        return `#pragma once

// 自動生成ファイル: 直接編集せず EnemyScripts.json を Tools/StageEditor.html で編集すること
// CommandとDefinitionの定義は SideScrollingShooterEnemies.h にある

namespace EnemyScript {
${lines.join("\n")}

    /** @brief スクリプト番号順の行動スクリプト一覧 */
    inline constexpr Definition Scripts[] = {
${definitions.join("\n")}
    };
    inline constexpr int ScriptCount = ${scripts.scripts.length};
}
`;
    }

    /**
     * @brief シートの内容を検証する
     * @param sheet 検証するシート
     * @param scriptNames スクリプト番号順のスクリプト名
     * @return なし（不正な場合は例外を投げる）
     */
    function ValidateSheet(sheet, scriptNames) {
        const fail = (message) => { throw new Error(`Stage${sheet.stage}${sheet.difficulty}: ${message}`); };
        // シート全体の値を検証する
        if (!Number.isInteger(sheet.stage) || sheet.stage < 1) fail("stageが不正");
        if (!Difficulties.includes(sheet.difficulty)) fail("difficultyが不正");
        if (!Number.isInteger(sheet.chapterLength) || sheet.chapterLength <= 0) fail("chapterLengthが不正");
        if (!Array.isArray(sheet.chapters) || sheet.chapters.length === 0) fail("chaptersが空");

        // 出現規則ごとの値を検証する
        sheet.chapters.forEach((chapter, ci) => {
            if (/[\r\n]/.test(chapter.comment || "")) fail(`チャプター${ci + 1}のコメントに改行がある`);
            chapter.rules.forEach((rule, ri) => {
                const at = `チャプター${ci + 1}の規則${ri + 1}`;
                if (!EnemyTypes.includes(rule.type)) fail(`${at}: 敵種別 ${rule.type} は未対応`);
                if (rule.type === "ScriptedEnemy" && !scriptNames.includes(rule.script)) {
                    fail(`${at}: スクリプト ${rule.script || "(未選択)"} が見つからない`);
                }
                if (!Number.isInteger(rule.firstFrame) || rule.firstFrame < 0) fail(`${at}: firstFrameが不正`);
                if (!Number.isInteger(rule.interval) || rule.interval < 0) fail(`${at}: intervalが不正`);
                for (const key of ["sideX", "railX", "y", "railZ"]) {
                    if (!Number.isFinite(rule[key])) fail(`${at}: ${key}が不正`);
                }
            });
        });
    }

    /**
     * @brief シートからC++ヘッダの文字列を生成する
     * @param sheet 生成元のシート
     * @param scriptNames スクリプト番号順のスクリプト名
     * @return ヘッダファイルの内容
     */
    function GenerateSheet(sheet, scriptNames) {
        ValidateSheet(sheet, scriptNames);
        const stage = `Stage${sheet.stage}`;
        const className = `${stage}EnemySheet${sheet.difficulty}`;
        const label = sheet.difficulty.toUpperCase();
        const lines = [];

        // チャプターごとの出現規則配列を出力する
        const chapterRefs = [];
        sheet.chapters.forEach((chapter, ci) => {
            if (chapter.comment) lines.push(`        // ${chapter.comment}`);
            if (chapter.rules.length === 0) {
                chapterRefs.push("Chapter{}");
                return;
            }
            lines.push(`        static constexpr EnemySpawnRule Chapter${ci + 1}[] = {`);
            chapter.rules.forEach((r) => {
                const scriptId = r.type === "ScriptedEnemy" ? scriptNames.indexOf(r.script) : 0;
                lines.push(`            {${r.type}, ${r.randomPosition ? "true" : "false"}, ${scriptId}, ` +
                    `${r.firstFrame}, ${r.interval}, ${FloatLiteral(r.sideX)}, ${FloatLiteral(r.railX)}, ` +
                    `${FloatLiteral(r.y)}, ${FloatLiteral(r.railZ)}},${r.type === "ScriptedEnemy" ? ` // ${r.script}` : ""}`);
            });
            lines.push("        };");
            chapterRefs.push(`MakeChapter(Chapter${ci + 1})`);
        });

        // 位置乱数化を使う規則がある場合だけ乱数処理を出力してFloppy版のコードを増やさない
        const usesRandom = sheet.chapters.some((c) => c.rules.some((r) => r.randomPosition));
        const randomBlock = !usesRandom ? "" : `
        // 位置乱数化を指定した敵はYとレールXをランダムに決める
        if (selected && spawn.randomPosition) {
            spawn.y = GameplayRandom::Range(-0.86f, 0.86f);
            spawn.railX = GameplayRandom::Range(-1.0f, 1.0f);
        }
`;

        return `#pragma once

// 自動生成ファイル: 直接編集せず ${className}.json を Tools/StageEditor.html で編集すること

#include "../../GameplayRandom.h"
#include "${stage}EnemySheet.h"

/**
 * @brief ${label}用の${stage}敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 位置乱数化, スクリプト番号, 開始フレーム,
 * 出現間隔, 2D出現X, レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::${className} final : public SideScrollingShooter::${stage}EnemySheet {
public:
    static constexpr int ChapterLength = ${sheet.chapterLength};

    /**
     * @brief 1チャプターの長さを取得する
     * @return ${label}用チャプターのフレーム数
     */
    int ChapterFrameLength() const override {
        return ChapterLength;
    }

    /**
     * @brief 指定フレームで出現させる${label}用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
${lines.join("\n")}
        constexpr Chapter Chapters[] = {
            ${chapterRefs.join(", ")}
        };
        const bool selected = TrySelectByChapters(Chapters, ${sheet.chapters.length}, ChapterFrameLength(),
            frame, spawnIndex, spawn, chapterNumber);
${randomBlock}        return selected;
    }
};
`;
    }

    exports.EnemyTypes = EnemyTypes;
    exports.Difficulties = Difficulties;
    exports.CommandSpecs = CommandSpecs;
    exports.GenerateScripts = GenerateScripts;
    exports.GenerateSheet = GenerateSheet;
    exports.SimulateScript = SimulateScript;

    // Nodeから直接実行された場合は全シートを検査または再生成する
    if (typeof require !== "undefined" && typeof module !== "undefined" && require.main === module) {
        const fs = require("fs");
        const path = require("path");
        const stagesDir = path.join(__dirname, "..", "Presentation", "Gameplay", "Stages");
        const write = process.argv.includes("--write");
        let mismatches = 0;

        // 生成結果と既存の.hを比べ、--writeなら書き換える
        const check = (jsonPath, generate, headerPath = jsonPath.replace(/\.json$/, ".h")) => {
            const expected = generate(jsonPath && JSON.parse(fs.readFileSync(jsonPath, "utf8")));
            const actual = fs.existsSync(headerPath) ? fs.readFileSync(headerPath, "utf8").replace(/\r\n/g, "\n") : "";
            if (actual === expected) return;
            if (write) {
                fs.writeFileSync(headerPath, expected.replace(/\n/g, "\r\n"));
                console.log(`generated ${path.relative(stagesDir, headerPath)}`);
            } else {
                console.error(`out of date: ${path.relative(stagesDir, headerPath)}`);
                ++mismatches;
            }
        };

        // スクリプト名はシートの生成で番号へ変換するため先に読み込む
        const scriptsPath = path.join(stagesDir, "Common", "EnemyScripts.json");
        const scriptNames = JSON.parse(fs.readFileSync(scriptsPath, "utf8")).scripts.map((s) => s.name);
        check(scriptsPath, GenerateScripts);
        check(null, GenerateParity, path.join(__dirname, "..", "Tests", "EnemyScriptParity.h"));
        for (const dir of fs.readdirSync(stagesDir)) {
            const fullDir = path.join(stagesDir, dir);
            if (!fs.statSync(fullDir).isDirectory()) continue;
            for (const file of fs.readdirSync(fullDir).filter((f) => /EnemySheet\w+\.json$/.test(f))) {
                check(path.join(fullDir, file), (sheet) => GenerateSheet(sheet, scriptNames));
            }
        }
        process.exit(mismatches === 0 ? 0 : 1);
    }
})(typeof module !== "undefined" ? module.exports : (window.StageSheetGenerator = {}));
