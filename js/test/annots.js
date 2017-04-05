"use strict";

const fs = require('fs')
    , path = require('path')
    , test = require('blue-tape')
    , concat = require('concat-stream')
    , parseAnnots = require('../annots')

function getPdfCreator(filename) {
  return filename.replace(/.*\//, '').replace('.pdf', '');
}

const cases = [
  {
    dir: './pdfs/0_none',
    expected: [],
    msg: 'should not find any annotations on an empty document'
  },

  {
    dir: './pdfs/1_comment',
    expected: [
      {
          page: 1,
          motivation: 'commenting',
          body_text: 'This is a comment',
      }
    ],
    msg: 'should extract comment annotations'
  },

  {
    dir: './pdfs/2_highlight',
    expected: [
      {
          page: 1,
          motivation: 'highlighting',
          highlighted_text: 'document',
      }
    ],
    msg: 'should extract highlight annotations'
  },

  {
    dir: './pdfs/3_highlight_multiline',
    expected: [
      {
          page: 1,
          motivation: 'highlighting',
          highlighted_text: 'that spans',
      }
    ],
    msg: 'should extract multiline highlight annotations'
  }
]

function deleteObjectID(annot) {
  delete annot.object_id;
  return annot;
}

test('Extracting annotations from PDFS', t => Promise.all(
  cases.map(({ dir, expected, msg }) =>
    new Promise((resolve, reject) =>
      fs.readdir(path.resolve(__dirname, dir), (err, pdfFiles) => {
        if (err) reject(err)

        const toAbsolutePath = f => path.resolve(__dirname, dir, f)

        const runTest = filename =>
          new Promise((resolve, reject) =>
            parseAnnots(filename)
              .on('error', reject)
              .pipe(concat(annots => {
                t.deepEqual(
                  annots.map(deleteObjectID),
                  expected,
                  `${msg} (${getPdfCreator(filename)}).`);

                resolve()
              })))

        resolve(Promise.all(pdfFiles.map(toAbsolutePath).map(runTest)))
      })
    )
  )
))
