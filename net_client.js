var net = require('net');
var gs = false;
var msl_next = false;
var d_next = false;
var sock = net.connect({port: 7070}, function() {
    console.log('connection established');
});
sock.on('data', function(buf) {
    console.log('got data of len ' + buf.length);
    if (msl_next) {
        var len = buf.readUInt32BE(0);
        console.log('len: ' + len);
        console.dir(buf);
        console.log('sent: ' + buf.toString('utf8', buf.length - len));
        gs = true;
        msl_next = false;
        d_next = false;
        sock.write('\x07', 'ascii');
        console.log('acked');
        return;
    }
    var keyw = buf.toString('ascii', 0, 1);
    console.log('data keyw: ' + keyw);
    if (keyw == '\x01') {
        console.log('ping! sending pong');
        sock.write('\x02', 'ascii');
        console.log('pong!');
    }
    else if (keyw == '\x03' && !gs) {
        console.log('MSG_SEND_CMD - sending get_sentence');
        var b = new Buffer(1);
        b.write('\x04', 'ascii');
        console.log('sending: ' + b);
        sock.write(b);
        console.log('sent!');
    }
    else if (keyw == '\x03') {
        console.log('MSG_SEND_CMD - sending MSG_TERMINATE');
        sock.write('\x08', 'ascii');
        console.log('sent');
    }
    else if (keyw == '\x06') {
        console.log('MSG_SENTENCE_LEN - expecting length');
        msl_next = true;
    }
    else if (keyw == '\x05') {
        console.log('MSG_SENTENCE_GENFAILED - server is shit');
        sock.write('\x07', 'ascii');
        console.log('sent ack');
        gs = true;
    }
});
sock.on('end', function() {
    console.log('connection closed');
});
