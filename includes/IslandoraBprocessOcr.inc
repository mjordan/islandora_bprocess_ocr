<?php

/**
 * @file
 * Class that integrates external OCR service with Islandora Background Process module.
 */

class IslandoraBprocessOcr extends IslandoraBackgroundProcess {
  // The value of this property should be the same as the function which implements
  // the hook that this process should be executed in.
  public $hook = 'islandora_background_process_islandora_datastream_ingested';

  public function __construct() {
    parent::__construct();
  }

  // All classes that extend IslandoraBackgroundProcess run their
  // code in their work() method. 
  public function work($object = NULL, $datastream = NULL) {
    if (is_null($datastream)) {
      return;
    }

    $rest_server_url = variable_get('islandora_bprocess_ocr_rest_server_url', 'http://example.com/ocr_rest/page/');
    $timeout = variable_get('islandora_bprocess_ocr_timeout', '240');

    // Check to make sure that the we only perform the work on images.
    $has_target_cmodel = FALSE;
    $wanted_models = array('islandora:sp_basic_image');
    $object_content_models = $object->relationships->get(FEDORA_MODEL_URI, 'hasModel');
    dd($object_content_models, 'Content models');
    foreach ($object_content_models as $cmodel) {
      if (($cmodel['predicate']['value'] == 'hasModel') &&
        in_array($cmodel['object']['value'], $wanted_models)) {
        $has_target_cmodel = TRUE;
      }
    }

    // Check to make sure that the we only perform the work on members of a specific collection.
    $in_target_collection = FALSE;
    $wanted_collections = array('islandora:sp_basic_image_collection');
    $object_collections = $object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
    dd($object_collections, 'Collections');
    foreach ($object_collections as $collection) {
      if (($collection['predicate']['value'] == 'isMemberOfCollection') &&
        in_array($collection['object']['value'], $wanted_collections)) {
        $in_target_collection = TRUE;
      }
    }

    if ($datastream->id == 'OBJ' && $has_target_cmodel && $in_target_collection) {
      $pid_dsid_string = $object->id . "/" . $datastream->id;
      $time_pre = microtime(true);
      dd("Starting remote process for datastream " . $pid_dsid_string);
      // Get datastream content and save it to a temp file.
      $mime_detect = new MimeDetect();
      $extension = $mime_detect->getExtension($datastream->mimetype);
      $clean_pid = preg_replace('/:/', '_', $object->id);
      $datastream_tmp_file_name = $clean_pid . '_' . $datastream->id . '.' . $extension;
      $datastream_tmp_file_path = file_directory_temp() . DIRECTORY_SEPARATOR . $datastream_tmp_file_name;
      $datastream_content = $object['OBJ']->getContent();
      file_put_contents($datastream_tmp_file_path, $datastream_content);

      // First request: curl -X PUT --data-binary @1978-03-29-07.tif $rest_server_url . '1978-03-29-07.tif'
      $put_url = $rest_server_url . $datastream_tmp_file_name;
      if ($put_result = islandora_background_proces_put_binary_file($put_url, $datastream_tmp_file_path)) {
        dd($put_result, 'PUT result');
      }
      else {
        dd('PUT failed.');
      }

      // Second request: curl -X GET -H 'Accept: text/plain' $rest_server_url . 'file.jpg' (text will be in response body)
      $text_transcript_url = $rest_server_url . $datastream_tmp_file_name;
      $text_transcript_options = array('method' => 'GET', 'timeout' => $timeout, 'headers' => array('Accept' => 'text/plain'));
      $text_transcript_result = drupal_http_request($text_transcript_url, $text_transcript_options);

      // Add the OCR datastream to the object
      $dsid = 'OCR';
      $datastream = isset($object[$dsid]) ? $object[$dsid] : $object->constructDatastream($dsid);
      $datastream->label = 'Transcript';
      $datastream->mimeType = 'text/plain';
      $datastream->setContentFromString($text_transcript_result->data);
      $object->ingestDatastream($datastream);

      // Third request: curl -X DELETE $rest_server_url . 'Hutchinson1794-1-0257.jpg'
      $delete_url = $rest_server_url . $datastream_tmp_file_name;
      $delete_options = array('method' => 'DELETE', 'timeout' => $timeout);
      $delete_result = drupal_http_request($delete_url, $delete_options);
      dd($delete_result, "DELETE result");

      // Log completion time.
      $time_post = microtime(true);
      $exec_time = $time_post - $time_pre;
      dd("Finished remote process for datastream " . $pid_dsid_string . ", which took " . 
        $exec_time . " seconds to complete");
    }
  }
}
