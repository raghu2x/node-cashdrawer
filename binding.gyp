{
  "targets": [
    {
      "target_name": "node_printer",
      "sources": [
        "src/addon.cc",
        "src/printers.cc",
        "src/cashdrawer.cc"
      ],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "conditions": [
        [
          'OS=="win"',
          {
            "libraries": [
              "-lwinspool"
            ],
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": 1
              }
            }
          }
        ],
        [
          'OS=="mac"',
          {
            "xcode_settings": {
              "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
              "OTHER_CFLAGS": ["-std=c++11"],
              "OTHER_LDFLAGS": [
                "-lcups"
              ]
            }
          }
        ],
        [
          'OS=="linux"',
          {
            "cflags": [
              "-std=c++11"
            ],
            "libraries": [
              "-lcups"
            ]
          }
        ]
      ]
    }
  ]
}