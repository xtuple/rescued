<?php

/**
 * @file
 * Defines the Inline Entity Form controller for RESCUED entities.
 */

class RescuedRelationInlineEntityFormController extends EntityInlineEntityFormController {

  /**
   * Overrides EntityInlineEntityFormController::labels().
   */
  public function labels() {
    $info = entity_get_info($this->entityType);

    $camel_split = preg_split('/(?=[A-Z])/', $info['resource']);
    $label = trim(implode(" ", $camel_split));

    $label = str_replace('Xd ', '', $label);

    $labels = array(
      'singular' => t($label),
      'plural' => t($label),
    );
    return $labels;
  }

  /**
   * Overrides EntityInlineEntityFormController::tableFields().
   *
   * We can't use the parent class method because the RESCUED entity metadata
   * wrapper doesn't have a property that matches the entity bundle key.
   * @todo: Remove this method once http://drupal.org/node/1662558 is fixed.
   */
  public function tableFields($bundles) {
    $fields = array();

    $info = entity_get_info($this->entityType);
    $metadata = entity_get_property_info($this->entityType);

    $label_key = $info['entity keys']['label'];
    $fields[$label_key] = array(
      'type' => 'property',
      'label' => $metadata ? $metadata['properties'][$label_key]['label'] : t('Label'),
      'weight' => 1,
    );

    return $fields;
  }

  /**
   * Overrides EntityInlineEntityFormController::settingsForm().
   */
  public function settingsForm($field, $instance) {
    $labels = $this->labels();
    $form = parent::settingsForm($field, $instance);

    // We'll recreate this below to put existing_widget field in the right order.
    unset($form['delete_references']);

    if ($field['type'] === 'xdruple_entityreference') {

      $existing_options = array();
      foreach (field_info_widget_types() as $widget_name => $widget) {
        if (isset($widget['field types'])) {
          foreach ($widget['field types'] as $type) {
            if ($type === $field['type'] && strpos($widget['module'],'inline_entity_form') === false) {
              $existing_options[$widget_name] = $widget['label'];
            }
          }
        }
      }
      $form['existing_widget'] = array(
        '#type' => 'select',
        '#title' => t('Add Existing Widget Type'),
        '#default_value' => isset($this->settings['existing_widget']) ? $this->settings['existing_widget'] : 'options_select',
        '#options' => $existing_options,
        '#description' => t('Select the widget type used to relate to existing entities.'),
        '#states' => array(
          'visible' => array(
            ':input[name="instance[widget][settings][type_settings][allow_existing]"]' => array('checked' => TRUE),
          ),
        ),
        '#element_validate' => array('inline_entity_form_field_widget_settings_existing_widget_validate'),
      );
    }

    $form['delete_references'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete referenced @label when the parent entity is deleted.', array('@label' => $labels['plural'])),
      '#default_value' => $this->settings['delete_references'],
    );

    return $form;
  }

  /**
   * Overrides EntityInlineEntityFormController::entityForm().
   */
  public function entityForm($entity_form, &$form_state) {
    $entity = $entity_form['#entity'];

    $defaults = array(
      'name' => '',
      'description' => '',
      'format' => NULL,
      'tid' => NULL,
      'weight' => 0,
    );
    foreach ($defaults as $key => $value) {
      if (!isset($entity->$key)) {
        $entity->$key = $value;
      }
    }

    $entityInfo = $entity->entityInfo();
    $handler_class = $entityInfo['operations ui']['operations'][$entity_form['#op']]['handler'];

    if (!(strpos($form_state['build_info']['form_id'], 'entity_operations_operation_form') === 0)) {
      // Set 'page title' = FALSE so we don't override the page title on
      // forms that include entity_operations entities in a IEF.
      $form_state['entity_operation_form_elements'] = array(
        'page title' => FALSE,
        'confirmation message' => TRUE,
        'form elements' => TRUE,
        'action button' => TRUE,
        'cancel link' => TRUE,
      );
    }

    $rescued_form = entity_operations_operation_form($entity_form, $form_state, $entity->entityType(), $handler_class, $entity_form['#op'], $entity);

    // TODO: Should we allows this?
    //$langcode = entity_language($entity->entityType(), $entity);
    //field_attach_form($entity->entityType(), $entity, $rescued_form, $form_state, $langcode);

    // Since the parent form has a different form_id, the child IEFs will never have hook_form_alter called.
    // This forces the child form through hook_form_alter before it's returned to the parent.
    $operation = new $handler_class($this->entityType, $entity_form['#op']);
    $form_id = $operation->getFormID($this->entityType, $entity_form['#op']);
    // Invoke hook_form_alter(), hook_form_BASE_FORM_ID_alter(), and
    // hook_form_FORM_ID_alter() implementations.
    $hooks = array('form');
    if (isset($form_state['build_info']['base_form_id'])) {
      $hooks[] = 'form_' . $form_state['build_info']['base_form_id'];
    }
    $hooks[] = 'form_' . $form_id;
    drupal_alter($hooks, $rescued_form, $form_state, $form_id);

    return $rescued_form;
  }


  /**
   * Overrides EntityInlineEntityFormController::entityFormSubmit().
   */
  public function entityFormSubmit(&$entity_form, &$form_state) {
    $entity = $entity_form['#entity'];
    $entityInfo = $entity->entityInfo();
    $resource = $entityInfo['resource'];
    $schema = $entityInfo['json-schema'][$resource]['properties'];

    if (!isset($entityInfo['is_nested_only'])) {
      // Build up a clean array of just the RESCUED resource properties from the entity.
      $original_entity = array();
      foreach ($schema as $key => $value) {
        $original_entity[$key] = $entity->$key;
      }
      // Set the original_entity so we can do a JSON-Patch diff after changes.
      // TODO: Is this needed anymore?
      //$entity->originalEntity = $original_entity;
    }

    // Build the entity to add the child relations from the submitted form and form_state.
    rescued_form_submit_build_entity($this->entityType, $entity, $entity_form, $form_state);

    $ief_id = isset($entity_form['#ief_id']) ? $entity_form['#ief_id'] : null;
    if (isset($ief_id)) {
      $instance = $form_state['inline_entity_form'][$ief_id]['instance'];
      $ief_type = $instance['widget']['type'];

      // inline_entity_form_field_attach_submit() calls field_attach_submit()
      // recursively for us on single widgets.
      if ($entityInfo['fieldable']) {
        if (isset($form_state['triggering_element']['#ief_processing']) &&
          !$form_state['triggering_element']['#ief_processing']) {
          // This is a different element that is requesting to skip IEF processing. Like an addressfield ajax update.
        } else {
          // Only call field_attach_submit() on inline_entity_form multiple widget, not single.
          if ($ief_type === 'inline_entity_form') {
            field_attach_submit($this->entityType, $entity, $entity_form, $form_state);
            $this->cleanupFieldFormState($entity_form, $form_state);
          }
        }
      }
    }

    // TODO: Not sure if we need this any more.
    // inline_entity_form_field_attach_submit() now calls field_attach_submit() recursively for us.
    if ($entityInfo['fieldable']) {
//       if (isset($form_state['triggering_element']['#ief_processing']) &&
//         !$form_state['triggering_element']['#ief_processing']) {
//         // This is a different element that is requesting to skip IEF processing. Like an addressfield ajax update.
//       } else {

//         $path_parts = array();
//         if (isset($entity_form['#ief_id'])) {
//           $path_parts = explode('--', $entity_form['#ief_id']);
//         }

//         if (count($path_parts) > 1) {
//           $ief_child = true;
//         } else {
//           $ief_child = false;
//         }

//         // Only call field_attach_submit() on the main root parent, not children.
//         if (!$ief_child) {
//           //field_attach_submit($this->entityType, $entity, $entity_form, $form_state);
//           //$this->cleanupFieldFormState($entity_form, $form_state);
//         }
//       }
    }

// TODO: The following was early code. Now we're creating Drupal Fields
// and using the xdruple_entityreference field type for this. This may no longer be needed.
    // Nested entities get saved with their parent.
    //if (!isset($entityInfo['is_nested_only']) && count($path_parts) > 1) {
      // Save the entity.
      // TODO: Do we need to save here at all???
      // Maybe not on xTuple entities that use IEF for relations. But we do for checkout fields.
      // Or recurse through the $form_state['inline-entity-form'] and update the children.

      // Need to do this until xTuple entity relations create with own field instances.
      //$entity->save();

      // This is a hack to get the right etag. Until rescued_controller updates the entity after a PATCH,
      // this will not be a valid etag without loading it agian.
      //$entity = entity_load_single($this->entityType, $entity->identifier());
    //}

    //$entity = $entity_form['#entity'];
  }

}
