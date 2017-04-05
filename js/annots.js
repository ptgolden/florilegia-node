"use strict";

const fs = require('fs')
    , path = require('path')
    , tmp = require('tmp')
    , through = require('through2')
    , onDeath = require('death')
    , streamingWorker = require('streaming-worker')
    , popplerAddon = require('bindings')('pdf2oac.node')

module.exports = function parseAnnots(pdfFilename, imageDirectory) {
  let tmpDirectory

  if (!imageDirectory) {
    tmpDirectory = tmp.dirSync();
    imageDirectory = tmpDirectory.name;
  }

  const worker = streamingWorker(popplerAddon.path, {
    pdfFilename,
    imageDirectory
  })

  function cleanUp() {
    worker.close();

    if (tmpDirectory) {
      setTimeout(() => {
          // process.stderr.write('Cleaning up temporary directory ' + tmpDirectory.name + '\n');

          fs.readdirSync(tmpDirectory.name).forEach(filename => {
            fs.unlinkSync(path.join(tmpDirectory.name, filename))
          })

          tmpDirectory.removeCallback();
          tmpDirectory = null;
      }, 0);
    }
  }

  onDeath(cleanUp);
  process.on('exit', cleanUp);

  return worker.from.stream()
    .pipe(through.obj(function (data, enc, cb) {
      this.push(JSON.parse(data[1]));
      cb();
    }))
    .on('end', cleanUp)
}
