"use strict";

const fs = require('fs')
    , path = require('path')
    , test = require('blue-tape')
    , concat = require('concat-stream')
    , { PNG } = require('node-png')
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
          target_text: 'document',
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
          target_text: 'that spans',
      }
    ],
    msg: 'should extract multiline highlight annotations'
  },

  {
    dir: './pdfs/4_stamp',
    transform: annots => {
      const parser = new PNG()

      if (!annots[0].stamp_bytes) {
        throw new Error('Stamp was not extracted.');
      }

      const rawPNG = Buffer.from(annots[0].stamp_bytes, 'base64')

      return new Promise((resolve, reject) =>
        parser.parse(rawPNG, (err, data) => {
          if (err) {
            reject('Could not parse PNG from stamp.');
          }

          resolve(true);
        })
      )
    },
    expected: true,
    msg: 'should extract stamps as PNG images'
  }
]

function deleteObjectID(annot) {
  delete annot.object_id;
  return annot;
}

test('Extracting annotations from PDFS', t => Promise.all(
  cases.map(({ dir, expected, transform, msg }) =>
    new Promise((resolve, reject) =>
      fs.readdir(path.resolve(__dirname, dir), (err, pdfFiles) => {
        if (err) reject(err)

        const toAbsolutePath = f => path.resolve(__dirname, dir, f)

        const runTest = async filename => {
          const annots = await new Promise((resolve, reject) =>
            parseAnnots(filename)
              .on('error', reject)
              .pipe(concat(resolve)))

          const cmp = await (transform
            ? transform(annots)
            : annots.map(deleteObjectID))

          return t.deepEqual(cmp, expected, `${msg} (${getPdfCreator(filename)}).`);
        }

        resolve(Promise.all(pdfFiles.map(toAbsolutePath).map(runTest)))
      })
    )
  )
))
