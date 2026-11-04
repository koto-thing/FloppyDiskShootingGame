// 保存データの読み込みが終わるまでmainを開始しない
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
    addRunDependency('saved-games');
    FS.mkdir('/save');
    FS.mount(IDBFS, { autoPersist: true }, '/save');
    FS.syncfs(true, function (error) {
        if (error) {
            Module.setStatus('Save storage could not be opened. Allow site storage and reload.');
            console.error(error);
            return;
        }
        removeRunDependency('saved-games');
    });
});
