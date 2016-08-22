var fs = require('fs');
var path = require('path');
var configPath = path.join(__dirname, 'config.json');
var blinkinoPath = path.join(__dirname, 'azureiothubsample.ino');
var blinkrunPath = path.join(__dirname, 'azureiothubsample_run.c');
var tasksPath = path.join(__dirname, '.vscode', 'tasks.json');

function replaceStringInFileSync(someFile, stringtobereplaced, replacement) {
    var data;
    try {
        data = fs.readFileSync(someFile, 'utf8');
    } catch (err) {
        console.error(err);
    }
    var result = data.replace(stringtobereplaced, replacement);
    try {
        fs.writeFileSync(someFile, result);
    } catch (err) {
        console.error(err);
    }
}

fs.readFile(configPath, 'utf8', function (err, data) {
    if (err) {
        console.error(err);
    } else {
        var obj = JSON.parse(data);
        
        // replace placeholders in .ino file
        if (obj.is_psw_needed == "true") {
            replaceStringInFileSync(blinkinoPath, /<PSW_NEEDED>/g, "#define PSW_NEEDED");
            replaceStringInFileSync(blinkinoPath, /<PSW>/g, obj.psw)
        } else {
            replaceStringInFileSync(blinkinoPath, /<PSW_NEEDED>/g, "")
        }
        replaceStringInFileSync(blinkinoPath, /<SSID>/g, obj.ssid)

        // replace placeholders in .c file
        // get iothub constr
        var constr = obj.iothub;
        // get DeviceID
        var pattern = new RegExp("DeviceId=\\w+");
        var arrMatches = constr.match(pattern);
        var deviceid = arrMatches[0].replace(/DeviceId=/g, "");
        replaceStringInFileSync(blinkrunPath, /<CONNECTIONSTRING>/g, constr);
        replaceStringInFileSync(blinkrunPath, /<DEVICEID>/g, deviceid);
        
        var arduinoPath = obj.arduino.replace(/\\/g, "\\\\");

        // replace placeholders in filetasks.json
        if (obj.os == "windows") {
            replaceStringInFileSync(tasksPath, /<WINDOWS_ARDUINO>/g, arduinoPath);
        } else if (obj.os == "linux") {
            replaceStringInFileSync(tasksPath, /<LINUX_ARDUINO>/g, arduinoPath);
        } else {
            replaceStringInFileSync(tasksPath, /<OSX_ARDUINO>/g, arduinoPath);
        }
        replaceStringInFileSync(tasksPath, /<BOARD>/g, obj.board);
        replaceStringInFileSync(tasksPath, /<PORT>/g, obj.port);
    }
});

console.log('Setup Finishes');