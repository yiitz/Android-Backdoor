var logger = require('tracer').dailyfile({
    root: './logs',
    maxLogFiles: 10
});
var fs = require('fs');


var initSocketServer = function() {
    var net = require('net');
    net.createServer(function(socket) {

        logger.info('connected: ' +
            socket.remoteAddress + ':' + socket.remotePort);

        var buffer = '';
        socket.on('data', function(data) {
			
            buffer += data.toString();
			if(buffer.endsWith(":")){
				buffer = buffer.substring(0,buffer.length-1);
			}else{
				return;
			}
            if (fs.existsSync(buffer)) {
                var size = fs.statSync(buffer).size;
				
                var buf = new Buffer(4);
                buf.fill(0);
                buf.writeUInt32LE(size, 0);
                socket.write(buf);
				
                var stream = fs.createReadStream(buffer, {
                    flags: "r",
                    encoding: null
                });
                stream.pipe(socket);
				
                logger.info('send file: ' +
                    buffer + ', size: ' + size);
            }else{
                var buf = new Buffer(4);
                buf.fill(0);
                buf.writeUInt32LE(1234567, 0);
                socket.write(buf);
			}

        });

        socket.on('close', function() {
            logger.info('closed: ' +
                socket.remoteAddress + ':' + socket.remotePort);
        });

    }).listen(5030);
}

initSocketServer();