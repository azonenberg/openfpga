var Module = {
    print: function(text) {
        postMessage(['stdout', text]);
    },
    printErr: function(text) {
        postMessage(['stderr', text]);
    },
    noInitialRun: true,
    noExitRuntime: true,
    onRuntimeInitialized: function() {
        postMessage(['ready']);
    }
}

let need_create_fs = true;

onmessage = function(e) {
    // Put the file in place
    if (!need_create_fs) {
        FS.unmount('/in');
    } else {
        FS.mkdir('/in');
        need_create_fs = false;
    }
    FS.mount(WORKERFS, {
        blobs: [{name: 'in.json', data: e.data[0]}],
    }, '/in');

    // Prepare arguments
    let argv = e.data[1];
    argv.unshift('gp4par');
    argv.push("-o");
    argv.push("/out.txt");
    argv.push("/in/in.json");

    // Encode arguments
    let textencoder = new TextEncoder('utf-8');
    let total_bytes_for_args = 0;
    let argv_utf8 = argv.map((x) => {
        let x_utf8 = textencoder.encode(x);
        total_bytes_for_args += x_utf8.length + 1;
        return x_utf8;
    });

    // Copy in args
    let argv_entries_in_em = Module._malloc(total_bytes_for_args);
    let argv_offset_tmp = argv_entries_in_em;
    let argv_entries_in_em_ptrs = argv_utf8.map((x) => {
        let ptr = argv_offset_tmp;
        Module.HEAPU8.set(x, ptr);
        Module.HEAPU8[ptr + x.length] = 0;
        argv_offset_tmp += x.length + 1;
        return ptr;
    });
    let argv_in_em = Module._malloc(4 * (argv_entries_in_em_ptrs.length + 1));
    for (let i = 0; i < argv_entries_in_em_ptrs.length; i++) {
        Module.setValue(argv_in_em + 4 * i, argv_entries_in_em_ptrs[i], 'i8*');
    }
    Module.setValue(argv_in_em + 4 * argv_entries_in_em_ptrs.length, 0, 'i8*');

    // Can run the program!
    Module._main(argv_entries_in_em_ptrs.length, argv_in_em);

    // Free the args
    Module._free(argv_entries_in_em);
    Module._free(argv_in_em);

    // Get the result
    let result = FS.readFile('/out.txt', {encoding: 'utf8'});
    postMessage(['result', result]);
}

importScripts('gp4par.js');
