"use strict";
var net = require('net');
var ready = false;
var len = 0;
var msl_next = false;
var s_next = false;
var sock = net.connect({port: 7070}, function() {
    console.log('connection established');
});
var cmds = {
    MSG_OHAI: '\x01',
    MSG_PONG: '\x02',
    MSG_SEND_CMD: '\x03',
    MSG_GET_SENTENCE: '\x04',
    MSG_SENTENCE_GENFAILED: '\x05',
    MSG_SENTENCE_LEN: '\x06',
    MSG_SENTENCE_ACK: '\x07',
    MSG_TERMINATE: '\x08'
};
var sendCommand = function(command) {
    let b = new Buffer(1);
    b.write(command, 'ascii');
    sock.write(b);
    return;
};
var parseLen = function(buf) {
    let len = lenbuf.readUInt32BE(0);
    console.log('len: ' + len);
    sbuf = new Buffer(len);
    lenbuf = null;
}
var parseSentence = function() {
    console.log('sentence: ' + sbuf.toString('utf8'));
    lenbuf = null
    lenbuf_written = 0;
    sbuf = null;
    sbuf_written = 0;
    sendCommand(cmds.MSG_SENTENCE_ACK);
    console.log('ack sent');
}
var getSentence = function() {
    lenbuf = null;
    lenbuf_written = 0;
    sbuf = null;
    sbuf_written = 0;
    sendCommand(cmds.MSG_GET_SENTENCE);
}
var lenbuf = null;
var lenbuf_written = 0;
var sbuf = null;
var sbuf_written = 0;
sock.on('data', function(buf) {
    console.log('got data of len ' + buf.length);
    ready = false;
    if (lenbuf) {
        lenbuf = Buffer.concat([lenbuf, buf]);
        lenbuf_written = lenbuf_written + buf.length;
        if (lenbuf_written >= lenbuf.length) return parseLen();
        return console.log(lenbuf_written + '/' + lenbuf.length);
    }
    if (sbuf) {
        sbuf = Buffer.concat([sbuf, buf]);
        sbuf_written = sbuf_written + buf.length;
        if (sbuf_written >= sbuf.length) return parseSentence(); 
        return console.log(sbuf_written + '/' + sbuf.length);
    }
    var keyw = buf.toString('ascii', 0, 1);
    console.log('data keyw: ' + keyw.charCodeAt(0));
    if (keyw == cmds.MSG_OHAI) {
        sendCommand(cmds.MSG_PONG);
        console.log('replied PONG to OHAI');
    }
    else if (keyw == '\x03') {
        ready = true;
    }
    else if (keyw == '\x06') {
        console.log('MSG_SENTENCE_LEN - expecting length');
        lenbuf = new Buffer(32);
    }
    else if (keyw == '\x05') {
        console.log('MSG_SENTENCE_GENFAILED - server is shit');
    }
});
sock.on('end', function() {
    console.log('connection closed');
});

setInterval(function() {
    if (!ready) return console.log('Not ready yet, skipping sentence request');
    console.log('Requesting sentence...');
    getSentence();
}, 2000);
