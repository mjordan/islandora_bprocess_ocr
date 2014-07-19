## Introduction

Proof of concept module to generate OCR from an Islandora OBJ datastream and add the OCR transcript to the object. Uses https://github.com/mjordan/ocr_rest as the external service.

See https://github.com/Islandora/Islandora-Preservation-Interest-Group/tree/master/background_services_discussion_paper for background.

## Dependencies

[OCR REST server](https://github.com/mjordan/ocr_rest) and [Background Process Integration Framework for Islandora](https://github.com/mjordan/islandora_background_process).

## Installation

Install and get the OCR REST server working, then install this module as usual; see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

Configure the URL of your OCR REST server, and some additional options, at admin/islandora/tools/bprocessocr.

##  Logging

Errors arising from interaction with the OCR server are logged in the Drupal watchdog, as asre the start and completion of adding the OCR datastream to an object.

