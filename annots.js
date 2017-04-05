"use strict";

const fs = require('fs')
    , path = require('path')
    , tmp = require('tmp')
    , through = require('through2')
    , onDeath = require('death')
    , streamingWorker = require('streaming-worker')
    , popplerAddon = require('bindings')('pdf2oac.node')

module.exports = function parseAnnots(opts) {
  const { pdfFilename } = opts

  let { imageDirectory } = opts
    , tmpDirectory

  if (!imageDirectory) {
    tmpDirectory = tmp.dirSync();
    imageDirectory = tmpDirectory.name;
    opts.imageDirectory = imageDirectory;
  }

  const worker = streamingWorker(popplerAddon.path, {
    pdfFilename,
    imageDirectory
  })

  function cleanUp() {
    if (tmpDirectory) {
      // process.stderr.write('Cleaning up temporary directory ' + tmpDirectory.name + '\n');

      fs.readdirSync(tmpDirectory.name).forEach(filename => {
        fs.unlinkSync(path.join(tmpDirectory.name, filename))
      })

      tmpDirectory.removeCallback();
      tmpDirectory = null;
    }
  }

  onDeath(cleanUp);
  process.on('exit', cleanUp);

  const stream = worker.from.stream()
    .pipe(through.obj(function ([,data], enc, cb) {
      this.push(JSON.parse(data));
      cb();
    }))

  stream.on('end', cleanUp)

  return stream;
}
