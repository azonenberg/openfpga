function runHandler() {
    let fileinput = document.getElementById('jsoninput');
    if (fileinput.files.length) {
        let cmdline = document.getElementById('cmdline').value;
        let args = cmdline.split(' ');
        worker.postMessage([fileinput.files[0], args]);
    }
}

let worker = new Worker('gp4par-html-worker.js');
worker.onmessage = function(e) {
    switch (e.data[0]) {
        case 'ready':
            let runbutton = document.getElementById('runbutton');
            runbutton.onclick = runHandler;
            break;
        case 'stdout':
            let stdout = document.getElementById('stdout');
            stdout.value += e.data[1] + '\n';
            stdout.scrollTop = stdout.scrollHeight;
            break;
        case 'stderr':
            let stderr = document.getElementById('stderr');
            stderr.value += e.data[1] + '\n';
            stderr.scrollTop = stderr.scrollHeight;
            break;
        case 'result':
            let bitoutput = document.getElementById('bitoutput');
            bitoutput.value = e.data[1];
            break;
        default:
            console.log(e.data);
            break;
    }
};
