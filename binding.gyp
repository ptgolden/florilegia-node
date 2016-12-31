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
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"src",

			],
			"cflags": [
				"-Wall",
				"-std=c++11",
				"<!@(pkg-config --cflags poppler)"
			]
		}
  ]
}
