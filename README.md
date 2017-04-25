# pdf2oac

Extract annotations from PDF files and convert them to the [Open Annotation
Core](http://www.openannotation.org/spec/core/) data model.

Requires libpoppler and a C compiler that supports C++11.

# Installation

pdf2oac requires running a patched version of libpoppler that is built from source. The patches allow handling stamp annotations and rendering annotations to images. They can be found in the `libpoppler/patches` directory.

To patch and build the patched libpoppler, and then build the Node extension, all you need to do is run `npm install`. However, for this to run successfully will require several dependencies to be installed through your OS's package manager:

  * libcairo
  * libglib2.0
  * libpixman
  * libfreetype2
  * libpng

# Node.js library

## Example

```js
const pdf2oac = require('pdf2oac')

pdf2oac('~/Documents/annotated_document.pdf')
  .on('data', triple => {
    console.log(triple);
  })
  .on('close', () => {
    console.log('all done!');
  })
```

## Methods

```js
const pdf2oac = require('pdf2oac')
```

### pdf2oac(pdfFilename, options={})

Returns a ReadableStream that will extract annotations from the given pdf and stream objects with `subject`, `predicate`, `object`, and possibly `graph` attributes.

Available options (all optional):

  * `imageDirectory`: pdf2oac uses the program [`pdfimages`](https://en.wikipedia.org/wiki/Pdfimages) (packaged with poppler-utils) to extract annotation images. If this option is provided, the given directory will be used to temporarily store the image files produced by `pdfimages`. If it is omitted, a temporary directory will be created automatically and deleted after completion.

  * `pdfURI`: The URI that will be used as the RDF resource representing the PDF file. Defaults to `file://$pdfFilename`.

  * `baseURI`: The base URI for all generated triples. Defaults to `http://example.org/#`.

  * `graphURI`: If provided, the returned stream will emit quads (`{ subject, predicate, object, graph }`) with the given graph URI rather than triples (`{ subject, predicate, object }`).


# Command line utility

```
pdf2oac ~/Documents/annotated_document.pdf --format turtle > annotations.ttl
```

Available options:

  * `--pdf-uri`: Same as pdfURI, above.

  * `--baseURI`: Same as baseURI, above.

  * `--graphURI`: Same as graphURI, above.

  * `-f`, `--format`: The output format. One of `turtle`, `trig`, `n-triples`, `n-quads`, or `json`. Default is `trig` if graph URI is provided, or `turtle` if not. `json` does *not* return JSON-LD. Rather, it outputs `{ subject, predicate, object, ?graph }` triples/quads.
