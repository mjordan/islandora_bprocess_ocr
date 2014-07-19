<?php

/**
 * @file
 * The admin form for the Islandora Background Process OCR Service module.
 */

/**
 * Admin settings form builder.
 */
function islandora_bprocess_ocr_admin_settings() {
  $form['islandora_bprocess_ocr_rest_server_url'] = array(
    '#title' => t('URL and path to the RESTful OCR server'),
    '#type' => 'textfield',
    '#size' => 60,
    '#default_value' => variable_get('islandora_bprocess_ocr_rest_server_url', 'http://example.com/ocr_rest/page/'),
    '#description' => t('Include the "http://" and include the trailing "/" in the path.'),
    '#required' => TRUE,
  );
  $form['islandora_bprocess_ocr_timeout'] = array(
    '#title' => t('Maximum timeout'),
    '#type' => 'textfield',
    '#size' => 10,
    '#default_value' => variable_get('islandora_bprocess_ocr_timeout', '240'),
    '#description' => t('Maximum number of seconds queries to the REST server can execute before timing out.'),
    '#required' => TRUE,
  );
  $form['islandora_bprocess_ocr_max_attempts'] = array(
    '#title' => t('Maximum attempts'),
    '#type' => 'textfield',
    '#size' => 10,
    '#default_value' => variable_get('islandora_bprocess_ocr_max_attempts', '3'),
    '#description' => t('Maximum number of attempts to query the REST server before the service stops trying.'),
    '#required' => TRUE,
  );
  $form['islandora_bprocess_target_cmodels'] = array(
    '#title' => t('Target content models'),
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => islandora_background_process_get_collections_cmodels('info:fedora/fedora-system:ContentModel-3.0'),
    '#default_value' => variable_get('islandora_bprocess_target_cmodels', array()),
    '#description' => t('Apply this service to objects with any of these content models.'),
    '#required' => TRUE,
  );
  $form['islandora_bprocess_target_collections'] = array(
    '#title' => t('Target collections'),
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => islandora_background_process_get_collections_cmodels('info:fedora/islandora:collectionCModel'),
    '#default_value' => variable_get('islandora_bprocess_target_collections', array()),
    '#description' => t('Apply this service to objects in any of these collections.'),
    '#required' => TRUE,
  );
  return system_settings_form($form);
}



