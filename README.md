# pdf2oac

Extract annotations from PDF files and convert them to RDF using the [Web Annotation Vocabulary](https://www.w3.org/TR/annotation-vocab/).

Requires C compiler that supports C++11.

# Installation

pdf2oac requires running a patched version of libpoppler that is built from source. The patches allow handling stamp annotations and rendering annotations to images. They can be found in the `libpoppler/patches` directory. To successfully build the project, you must have a C compiler that supports C++11, as well several packages that should be installed through your operating system's package manager:

  * libcairo
  * libglib2.0
  * libpixman
  * libfreetype2
  * libpng

Somewhat ridiculously, it also relies on a system-provided package for libpoppler's glib bindings (libpoppler-glib).

On Ubuntu 18.04, all of these requirements can be installed by running the command:

```
sudo apt install libcairo2-dev libglib2.0-dev libpixman-1-dev libfreetype6-dev libpng-dev libpoppler-glib-dev
```

Installation is done through npm, using the command `npm install`.

# Node.js library

## Example

```js
const pdf2oac = require('pdf2oac')

pdf2oac('~/Documents/annotated_document.pdf')
  .on('data', triple => {
    console.log(triple);
  })
  .on('close', () => {
    console.log('No more annotations');
  })
```

## Methods

```js
const pdf2oac = require('pdf2oac')
```

### pdf2oac(pdfFilename, options={})

Returns a ReadableStream that will extract annotations from the given pdf and stream objects with `subject`, `predicate`, `object`, and possibly `graph` attributes.

Available options:

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
