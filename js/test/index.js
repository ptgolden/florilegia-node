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

test('Extracting annotations', t => {
  cases.forEach(({ dir, expected, msg }) => {
    const pdfs = fs.readdirSync(path.join(__dirname, dir))
      .map(p => path.resolve(__dirname, dir, p))

    function runTest(filename) {
      return data =>
        t.deepEqual(data.map(annot => {
          delete annot.object_id;
          return annot;
        }),
        expected,
        `${msg} (${getPdfCreator(filename)}).`)
    }

    pdfs.forEach(pdfFilename => {
      parseAnnots({ pdfFilename }).pipe(concat(runTest(pdfFilename)))
    })
  })

  return Promise.resolve()
})
