"use strict";

const pump = require('pump')
    , through = require('through2')
    , streamingWorker = require('streaming-worker')
    , popplerAddon = require('bindings')('pdf2oac.node')

module.exports = function parseAnnots(pdfFilename) {
  const worker = streamingWorker(popplerAddon.path, { pdfFilename })

  return pump(
    worker.from.stream(),
    through.obj(function (data, enc, cb) {
      this.push(JSON.parse(data[1]));
      cb();
    }),
    err => {
      if (err) throw err;
    })
}
