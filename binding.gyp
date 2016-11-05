{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"src/pdf2oac.cc"
			],
			"libraries": [
				"<!@(pkg-config --libs poppler,redland,raptor2)"
			],
			"cflags": [
				"<!@(pkg-config --cflags poppler,redland,raptor2)"
			]
		}
  ]
}
