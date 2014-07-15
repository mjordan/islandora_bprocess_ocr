## Introduction

Proof of concept module to generate OCR from an Islandora OBJ datastream and add the OCR transcript to the object. Uses https://github.com/mjordan/ocr_rest as the external service.

See https://github.com/Islandora/Islandora-Preservation-Interest-Group/tree/master/background_services_discussion_paper for background.

## Dependencies

[OCR REST server](https://github.com/mjordan/ocr_rest) and [Background Process Integration Framework for Islandora](https://github.com/mjordan/islandora_background_process).

## Installation

Install and get the OCR REST server working, then install this module as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

Go to admin/islandora/tools/bprocessocr and add the URL of your OCR REST server.

