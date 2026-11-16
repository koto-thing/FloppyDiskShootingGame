// 敵出現シートのJSONからC++ヘッダを生成する
// ブラウザでは Tools/StageEditor.html から読み込み、Nodeでは単体のCLIとして動く
//   node Tools/StageSheetGenerator.js          JSONと.hが一致しているか検査する
//   node Tools/StageSheetGenerator.js --write  JSONから.hを再生成する
(function (exports) {
    "use strict";

    /** @brief 雑魚敵として配置できる敵構成種別（Stage::EnemyTypeの名前） */
    const EnemyTypes = [
        "BasicEnemy", "HeavyEnemy", "StraightShooterEnemy", "ArmoredEnemy",
        "CircleShooterEnemy", "SquareShooterEnemy", "DiveRusherEnemy",
        "MissileShooterEnemy", "LinkedLaserEnemy", "WallSecurityDroneEnemy"
    ];
    const Difficulties = ["Easy", "Normal", "Hard"];

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
     * @brief シートの内容を検証する
     * @param sheet 検証するシート
     * @return なし（不正な場合は例外を投げる）
     */
    function Validate(sheet) {
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
     * @return ヘッダファイルの内容
     */
    function Generate(sheet) {
        Validate(sheet);
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
                lines.push(`            {${r.type}, ${r.randomPosition ? "true" : "false"}, ${r.firstFrame}, ${r.interval}, ` +
                    `${FloatLiteral(r.sideX)}, ${FloatLiteral(r.railX)}, ${FloatLiteral(r.y)}, ${FloatLiteral(r.railZ)}},`);
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
 * EnemySpawnRuleは { 敵構成種別, 位置乱数化, 開始フレーム, 出現間隔,
 * 2D出現X, レール出現X, 出現Y, レール出現Z } の順で指定する
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
    exports.Generate = Generate;

    // Nodeから直接実行された場合は全シートを検査または再生成する
    if (typeof require !== "undefined" && typeof module !== "undefined" && require.main === module) {
        const fs = require("fs");
        const path = require("path");
        const stagesDir = path.join(__dirname, "..", "Presentation", "Gameplay", "Stages");
        const write = process.argv.includes("--write");
        let mismatches = 0;
        for (const dir of fs.readdirSync(stagesDir)) {
            const fullDir = path.join(stagesDir, dir);
            if (!fs.statSync(fullDir).isDirectory()) continue;
            for (const file of fs.readdirSync(fullDir).filter((f) => /EnemySheet\w+\.json$/.test(f))) {
                const jsonPath = path.join(fullDir, file);
                const headerPath = jsonPath.replace(/\.json$/, ".h");
                const expected = Generate(JSON.parse(fs.readFileSync(jsonPath, "utf8")));
                const actual = fs.existsSync(headerPath) ? fs.readFileSync(headerPath, "utf8").replace(/\r\n/g, "\n") : "";
                if (actual === expected) continue;
                if (write) {
                    fs.writeFileSync(headerPath, expected.replace(/\n/g, "\r\n"));
                    console.log(`generated ${path.relative(stagesDir, headerPath)}`);
                } else {
                    console.error(`out of date: ${path.relative(stagesDir, headerPath)}`);
                    ++mismatches;
                }
            }
        }
        process.exit(mismatches === 0 ? 0 : 1);
    }
})(typeof module !== "undefined" ? module.exports : (window.StageSheetGenerator = {}));
