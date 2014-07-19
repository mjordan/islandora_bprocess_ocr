<?php

/**
 * @file
 * Class that integrates external OCR service with Islandora Background Process module.
 */

class IslandoraBprocessOcr extends IslandoraBackgroundProcess {
  // The value of the $hook property should be the same as the function name which
  // implements the hook that this process should be executed in.
  public $hook = 'islandora_background_process_islandora_datastream_ingested';

  // Counter for number of attempts to perform the work() method before giving up.
  static $numAttempts = 0;

  public function __construct() {
    // Call IslandoraBackgroundProcess's contructor to register IslandoraBprocessOcr in
    // it list of children.
    parent::__construct();
  }

  // All classes that extend IslandoraBackgroundProcess run their
  // code in their work() method. 
  public function work($islandora_object = NULL, $datastream = NULL) {
    $pid_dsid_string = $islandora_object->id . "/" . $datastream->id;

    if (is_null($datastream)) {
      // Since this method is running in its own process, just log and exit.
      islandora_background_process_log("Invalide datastream passed into work() method while
        processing object %pid", array('%pid' => $object->id));
      exit;
    }

    // Check to make sure that the we only perform the work on object that have the
    // configured content models.
    $has_target_cmodel = FALSE;
    $wanted_models = variable_get('islandora_bprocess__ocr_target_cmodels', array());
    $object_content_models = $islandora_object->relationships->get(FEDORA_MODEL_URI, 'hasModel');
    foreach ($object_content_models as $cmodel) {
      if (($cmodel['predicate']['value'] == 'hasModel') &&
        array_key_exists($cmodel['object']['value'], $wanted_models)) {
        $has_target_cmodel = TRUE;
      }
    }

    // Check to make sure that the we only perform the work on members of the configured
    // collections.
    $in_target_collection = FALSE;
    $wanted_collections = variable_get('islandora_bprocess_ocr_target_collections', array());
    $object_collections = $islandora_object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection');
    foreach ($object_collections as $collection) {
      if (($collection['predicate']['value'] == 'isMemberOfCollection') &&
        array_key_exists($collection['object']['value'], $wanted_collections)) {
        $in_target_collection = TRUE;
      }
    }

    // This particular service uses the OBJ datastream as input.
    if ($datastream->id == 'OBJ' && $has_target_cmodel && $in_target_collection) {
      $rest_server_url = variable_get('islandora_bprocess_ocr_rest_server_url', 'http://example.com/ocr_rest/page/');
      $timeout = variable_get('islandora_bprocess_ocr_timeout', '240');

      // Check to see if we've reached our maximum number of attempts. If we have, log the
      // condition and exit.
      $this->numAttempts++;
      if ($this->numAttempts > (int) variable_get('islandora_bprocess_ocr_max_attempts', '3')) {
        islandora_background_process_log("Reached maximum number of attempts trying to perfom work
          for %item, so quiting", array('%item' => $pid_dsid_string));
        exit;
      }

      // Copy $islandora_object to use when we retry work() on error.
      $islandora_object_copy = clone $islandora_object;

      islandora_background_process_log("Starting remote process for datastream %item",
        array('%item' => $pid_dsid_string), WATCHDOG_INFO);

      $time_pre = microtime(true);
      // Get datastream content and save it to a temp file.
      $mime_detect = new MimeDetect();
      $extension = $mime_detect->getExtension($datastream->mimetype);
      $clean_pid = preg_replace('/:/', '_', $islandora_object->id);
      $datastream_tmp_file_name = $clean_pid . '_' . $datastream->id . '.' . $extension;
      $datastream_tmp_file_path = file_directory_temp() . DIRECTORY_SEPARATOR . $datastream_tmp_file_name;
      $datastream_content = $islandora_object['OBJ']->getContent();
      file_put_contents($datastream_tmp_file_path, $datastream_content);

      // First request: curl -X PUT --data-binary @1978-03-29-07.tif $rest_server_url . '1978-03-29-07.tif'
      $put_url = $rest_server_url . $datastream_tmp_file_name;
      $put_result = islandora_background_process_put_binary_file($put_url, $datastream_tmp_file_path);
      if (!$put_result || $put_result['http_code'] != '201') {
        islandora_background_process_log("PUT failed on attempt %attempt for %item",
          array('%attempt' => $this->numAttempts, '%item' => $pid_dsid_string));
        $this->work($islandora_object_copy, $datastream);
      }

      // Second request: curl -X GET -H 'Accept: text/plain' $rest_server_url . '1978-03-29-07.tif'
      // (text will be in response body).
      $text_transcript_url = $rest_server_url . $datastream_tmp_file_name;
      $text_transcript_options = array('method' => 'GET', 'timeout' => $timeout, 'headers' => array('Accept' => 'text/plain'));
      $text_transcript_result = drupal_http_request($text_transcript_url, $text_transcript_options);
      if (!$text_transcript_result || $text_transcript_result->code != '200') {
        islandora_background_process_log("GET for plain text failed on attempt %attempt for %item",
          array('%attempt' => $this->numAttempts, '%item' => $pid_dsid_string));
        $this->work($islandora_object_copy, $datastream);
      }

      // Add the OCR datastream to the object.
      $dsid = 'OCR';
      $ocr_ds = isset($islandora_object[$dsid]) ? $islandora_object[$dsid] : $islandora_object->constructDatastream($dsid);
      $ocr_ds->label = 'Transcript';
      $ocr_ds->mimeType = 'text/plain';
      $ocr_ds->setContentFromString($text_transcript_result->data);
      try {
        $islandora_object->ingestDatastream($ocr_ds);
      }
      catch (Exception $e) {
        islandora_background_process_log("Failed to add OCR datastream for %item: %error",
          array('%item' => $pid_dsid_string, '%error' => $e->getMessage()));
      }

      // Third request: curl -X DELETE $rest_server_url . '1978-03-29-07.tif'.
      $delete_url = $rest_server_url . $datastream_tmp_file_name;
      $delete_options = array('method' => 'DELETE', 'timeout' => $timeout);
      $delete_result = drupal_http_request($delete_url, $delete_options);
      if (!$delete_result || $delete_result->code != '200') {
        islandora_background_process_log("DELETE failed on attempt %attempt for %item",
          array('%attempt' => $this->numAttempts, '%item' => $pid_dsid_string));
        $this->work($islandora_object_copy, $datastream);
      }

      // Log completion time.
      $time_post = microtime(true);
      $exec_time = $time_post - $time_pre;
      islandora_background_process_log("Finished remote process for datastream %item, which took %seconds seconds to complete",
        array('%item' => $pid_dsid_string, '%seconds' => $exec_time), WATCHDOG_INFO);
    }
  }
}
