{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"src/pdf2oac.cc"
			],
			"libraries": [
				"<!@(pkg-config --libs poppler)"
			],
			"cflags": [
				"<!@(pkg-config --cflags poppler)"
			]
		}
  ]
}
