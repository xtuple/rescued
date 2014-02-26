<?php

/**
 * @file
 * Contains handler classes for RESCUED entity operations:
 *
 *  - RescuedEntityOperationsOperationEditGeneric: the edit form for the entity.
 *  - RescuedEntityOperationsOperationAddGeneric: the add form for the entity.
 */

/**
 * Operation handler for editing a RESCUED entity: generic form.
 *
 * @See: EntityOperationsOperationForm
 */
class RescuedEntityOperationsOperationEditGeneric extends EntityOperationsOperationForm {

  /**
   * Returns basic information about the operation.
   */
  function operationInfo() {
    return array(
      'label' => 'Edit',
      'description' => 'Produces a generic form to edit the entity.',
    ) + parent::operationInfo();
  }

  /**
   * Returns strings for the operations.
   */
  function operationStrings() {
    return array(
      'tab title' => 'Edit',
      'label' => 'Edit',
      'description' => 'Produces a form to edit the entity.',
      'page title' => t('Edit %label'),
      'button label' => t('Save'),
      // This appears to be sufficient to result in the markup form element
      // not being created.
      'confirm question' => NULL,
      'submit message' => t('The @entity-type %label has been saved.'),
      // These intentionally have no replacements for the placeholders; these
      // are replaced in getOperationString().
    );
  }

  /**
   * Form builder for this operation.
   */
  function form($form, &$form_state, $entity_type, $entity, $operation_path) {
    // Kill the cancel link.
    unset($form_state['entity_operation_form_elements']['cancel link']);

    return $this->entityForm($form, $form_state, $entity_type, $entity, $operation_path);
  }

  /**
   * Helper for form(), building just the entity form.
   *
   * Keeps the entity form separate from any wrapping logic and allows clearer
   * reuse by subclasses.
   *
   * Note that overriding this will cause inheritance problems because this is
   * also used by the EntityOperationsOperationAddGeneric handler. If you want
   * to customize the form for your entity, you may want to consider using
   * the EntityOperationsOperationEdit handler instead.
   */
  function entityForm($form, &$form_state, $entity_type, $entity, $operation_path) {
    $entityInfo = $entity->entityInfo();

    // Ripped from entity_ui_form_defaults().
    $defaults = array(
      'entity_type' => $this->entityType,
    );
    if (isset($entity)) {
      $defaults[$this->entityType] = $entity;
    }
    if (isset($operation_path)) {
      $defaults['op'] = $operation_path;
    }
    $form_state += $defaults;

    $delta = isset($get_delta) ? $get_delta : 0;
    foreach ($entityInfo['json-schema'][$entityInfo['resource']]['properties'] as $key => $properties) {
      if ((isset($properties['$ref']) && strpos($properties['$ref'], '/') === false) ||
        ((isset($properties['items']) && isset($properties['items']['$ref']) &&
        strpos($properties['items']['$ref'], '/') === false ))
      ) {
//         // Handle toOne and toMany nested relations.
//         if (isset($properties['$ref'])) {
//           $relation_resource = $properties['$ref'];
//           $cardinality = 1;
//           $weight = -20; // Move toOne relations down.
//         } else {
//           $relation_resource = $properties['items']['$ref'];
//           $cardinality = -1; // -1 means unlimited relations.
//           $weight = -10; // Move toMany relations to the bottom.
//         }

//         $relation_entityType = strtolower($relation_resource);
//         $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
//         $child_entityInfo = entity_get_info($child_entityType);
//         $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];

//         $class = $child_entityInfo['entity class'];
//         $child_data = null;
//         if (property_exists($entity, $key)) {
//           $child_data = json_decode(json_encode($entity->{$key}), true);
//         }
//         if (!is_array($child_data) && (is_string($child_data)) || is_null($child_data)) {
//           // If there's no data, we create a null array.
//           $child_data = array($child_entityInfo['entity keys']['id'] => $child_data);
//         }

//         $child_entity_operations = entity_operations_get_operation_info($child_entityType);

//         $handler_class = $child_entity_operations['edit']['handler'];
//         $params = array();

//         // If is_nested_only is set, this removes the "Add existing ..." button.
//         $allow_existing = isset($child_entityInfo['is_nested_only']) ? false : true;

//         $parents = isset($form['#parents']) ? $form['#parents'] : array();

//         // TODO: Consider adding a hook_field_info() for this we can fetch.
//         $field = array(
//           'active' => 1,
//           'bundles' => $entityInfo['bundles'],
//           'cardinality' => $cardinality,
//           'columns' => array_keys($relation_schema),
//           'deleted' => 0,
//           'field_name' => 'rescued_dummy_field',
//           'locked' => 0,
//           'module' => 'entityreference',
//           'settings' => array(
//             'target_type' => $child_entityType,
//           ),
//           'type' => 'entityreference',
//         );
//         if (isset($form_state['complete form']) && isset($form_state['complete form'][$key]) &&
//           isset($form_state['complete form'][$key]['#ief_id'])
//         ) {
//           $id = $form_state['complete form'][$key]['#ief_id'];
//         } else {
//           $id = str_replace('.', '', uniqid('', true));
//         }

//         $id = $delta;
//         $items = array();

//         // If the child can be deleted and viewed, it's a full resource and we want IEF
//         // to call entity_load() so the child will have an etag and can be edited inline.
//         if (isset($child_entityInfo['operations ui']['operations']['delete'])
//           && isset($child_entityInfo['operations ui']['operations']['view'])
//         ) {
//           $nested = false;

//           // Set $items to the id value.
//           if (isset($properties['$ref'])) {
//             if (isset($child_data[$child_entityInfo['entity keys']['id']])) {
//               $items[] = array($child_entityInfo['entity keys']['id'] => $child_data[$child_entityInfo['entity keys']['id']]);
//             }
//           } else {
//             foreach ($child_data as $toMany_key => $toMany_data) {
//               if (isset($child_data[$toMany_key][$child_entityInfo['entity keys']['id']])) {
//                 $items[] = array($child_entityInfo['entity keys']['id'] => $child_data[$toMany_key][$child_entityInfo['entity keys']['id']]);
//               }
//             }
//           }
//         } else {
//           $nested = true;

//           // Embed the child entity into the items array.
//           if (isset($properties['$ref'])) {
//             if (isset($child_data[$child_entityInfo['entity keys']['id']])) {
//               $items[] = new $class($child_data, $child_entityType);
//             }
//           } else {
//             foreach ($child_data as $toMany_key => $toMany_data) {
//               if (isset($child_data[$toMany_key][$child_entityInfo['entity keys']['id']])) {
//                 $items[] = new $class($child_data[$toMany_key], $child_entityType);
//               }
//             }
//           }
//         }

//         // TODO: Consider adding a hook_field_info() for this we can fetch.
//         $instance = array(
//           'id' => $id,
//           'label' => check_plain($properties['title']),
//           'settings' => array(),
//           'entity_type' => $child_entityType,
//           'bundle' => $child_entityType,
//           'deleted' => 0,
//           // TODO: Need to set this for inline_entity_form_get_controller().
//           // Consider defining a dynamic hook_field_info() that would set a
//           // name for each field in the schema.
//           'field_name' => 'rescued_dummy_field',
//           'required' => isset($properties['required']) ? $properties['required'] : FALSE,
//           'rescued' => array(
//             'entity_type' => $child_entityType,
//             'column' => $child_entityInfo['entity keys']['id'],
//           ),
//           'widget' => array(
//             'active' => 1,
//             'module' => 'inline_entity_form',
//             'settings' => array(
//               // TODO: Do we need to set fields here?
//               'fields' => array(),
//               'type_settings' => array(
//                 'allow_existing' => $allow_existing,
//                 'match_operator' => 'CONTAINS',
//                 // TODO: Might want to allow this, but not on isNested only resources.
//                 'delete_references' => 0,
//               ),
//             ),
//             'type' => 'inline_entity_form',
//             'weight' => 0,
//           ),
//         );

//         $element = array(
//           '#entity' => $entity,
//           '#entity_type' => $entity->entityType(),
//           '#bundle' => $entity->entityType(),
//           '#field_name' => $key,
//           '#language' => LANGUAGE_NONE,
//           '#field_parents' => $parents,
//           '#columns' => array_keys($relation_schema),
//           '#title' => check_plain($properties['title']),
//           '#description' => field_filter_xss(isset($properties['description']) ? $properties['description'] : ''),
//           '#weight' => $weight,
//           // Only the first widget should be required.
//           // TODO: $properties['required'] is not normally set on this type of relation.
//           // Do we need to set this for Inline Entity Forms?
//           '#required' => $delta == 0 && isset($properties['required']) ? $properties['required'] : FALSE,
//           '#delta' => $delta,
//         );

//         // TODO: At some point it would be nice to link non-editable relation
//         // children to their full resource. e.g. ContactRelation is not editable.
//         // However, if there was an "Open" button instead of "Edit", it could
//         // send the user to the Contact, which they could edit.

//         // TODO: This is using a customized inline_entity_form_field_widget_form() that adds a 'true' parameter to the call.
//         // That allows us to send the entities in $items so they do not need to be loaded.
//         if ($element = inline_entity_form_field_widget_form($form, $form_state, $field, $instance, LANGUAGE_NONE, $items, $delta, $element, $nested)) {
//           // TODO: Consider allowing the alter below.
//           // Allow modules to alter the field widget form element.
//           // $context = array(
//           //   'form' => $form,
//           //   'field' => $field,
//           //   'instance' => $instance,
//           //   'langcode' => LANGUAGE_NONE,
//           //   'items' => $items,
//           //   'delta' => $delta,
//           // );
//           // drupal_alter(array('field_widget_form', 'field_widget_' . $instance['widget']['type'] . '_form'), $element, $form_state, $context);

//           // If we're processing a specific delta value for a field where the
//           // field module handles multiples, set the delta in the result.
//           // For fields that handle their own processing, we can't make
//           // assumptions about how the field is structured, just merge in the
//           // returned element.
//           if (field_behaviors_widget('multiple values', $instance) == FIELD_BEHAVIOR_DEFAULT) {
//             $elements[$delta] = $element;
//           }
//           else {
//             $elements = $element;
//           }
//         }

//         $form[$key] = $elements;
      } elseif (isset($properties['$ref']) && strpos($properties['$ref'], '/') !== false) {
        // Handle toOne key relations.
        $relation_resource = explode("/", $properties['$ref']);
        $relation_key = $relation_resource[1];
        $relation_entityType = strtolower($relation_resource[0]);
        $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
        $child_entityInfo = entity_get_info($child_entityType);
        $relation_schema = $child_entityInfo['json-schema'][$relation_resource[0]]['properties'];

        // Get select list options for the resource list.
        //$resources = rescued_api_resource_list($child_entityInfo['discovery-client'], $relation_resource[0]);
        $resources = entity_load($child_entityType, FALSE);

        $options = array();
        foreach ($resources as $child_data) {
          //$options[$child_data[$relation_key]] = t($child_data[$relation_key]);
          $options[$child_data->{$relation_key}] = t($child_data->{$relation_key});
        }

        $form[$key] = array(
          '#type' => 'select',
          '#required' => isset($properties['required']) ? $properties['required'] : FALSE,
          '#title' => t(isset($properties['title']) ? $properties['title'] : ""),
          '#options' => $options,
          '#weight' => -40,
          '#default_value' => isset($entity->$key) ? $entity->$key : "",
          '#empty_value' => '',
        );
      } elseif (isset($properties['items']) && isset($properties['items']['$ref']) &&
      strpos($properties['items']['$ref'], '/') !== false) {
        // Handle toMany key relations.
        $relation_resource = explode("/", $properties['items']['$ref']);
        $relation_key = $relation_resource[1];

        $form[$key] = array(
          '#type' => 'container',
          '#attributes' => array(),
          '#weight' => -30,
          '#tree' => true,
          '#language' => LANGUAGE_NONE,
          LANGUAGE_NONE => array(
            '#entity' => $entity,
            '#entity_type' => $entity->entityType(),
            '#bundle' => $entity->entityType(),
            '#field_name' => $key,
            '#language' => LANGUAGE_NONE,
            '#field_parents' => array(),
            '#columns' => array($relation_key),
            '#title' => $properties['title'],
            '#description' => '',
            '#required' => isset($properties['required']) ? $properties['required'] : FALSE,
            '#delta' => 0,
            '#type' => 'textfield',
            '#maxlength' => 2048, // This might need to be longer.
            '#default_value' => isset($entity->$key) ? (is_array($entity->$key) ?  implode(", ", $entity->$key) : $entity->$key) : "",
            '#autocomplete_path' => $entityInfo['discovery-client'] . '/autocomplete/' . $entityInfo['discovery-client'] . '/' . $relation_resource[0] . '/' . $relation_key,
            '#size' => 60,
            //'#element_validate' => array('some_search_callback'), // TODO: Add validation function.
            '#after_build' => array('field_form_element_after_build'),
          ),
          '#access' => true,
        );
      } else {
        // Handle basic property.

        // Get 'required' from the JSON-Schema.
        $required = false;
        if (isset($properties['required'])) {
          // A boolean type can't be required = true or leaving it uncheck would fail validation.
          if ($properties['type'] !== 'boolean') {
            $required = $properties['required'];
          }
        }

        // Make the key property read only.
        if (isset($properties['isKey'])) {
          if (isset($entity->is_new) && $entity->is_new) {
            $required = false;
          }
          $form[$key] = array(
            '#type' => 'item',
            '#required' => $required,
            '#title' => t(isset($properties['title']) ? $properties['title'] : ""),
            '#weight' => -40,
            '#default_value' => isset($entity->$key) ? $entity->$key : "",
            '#markup' => isset($entity->$key) ? $entity->$key : "",
          );
        } else {
          $form[$key] = array(
            '#type' => rescued_json_schema_to_form_type($properties['type']),
            '#required' => $required,
            '#title' => t(isset($properties['title']) ? $properties['title'] : ""),
            '#weight' => -40,
            '#default_value' => isset($entity->$key) ? $entity->$key : "",
          );
        }
      }
      $delta++;
    }

    $context = array(
      'entity' => $entity,
      'entity_type' => $entity_type,
      'operation_path' => $operation_path,
    );
    drupal_alter('rescued_operation_entityform', $form, $form_state, $context);

    if (!empty($entityInfo['fieldable'])) {
      field_attach_form($this->entityType, $entity, $form, $form_state);
    }

    return $form;
  }

  /**
   * Form validation handler for this operation.
   *
   * Receives the same parameters as the form builder as a convenience.
   */
  function formValidate($form, &$form_state, $entity_type, $entity, $operation_path) {
    // Notify field widgets to validate their data.
    entity_form_field_validate($this->entityType, $form, $form_state);
  }

  /**
   * Form submit handler for this operation.
   *
   * Receives the same parameters as the form builder as a convenience.
   */
  function formSubmit($form, &$form_state, $entity_type, $entity, $operation_path) {
    $entity_info = entity_get_info($entity_type);

    // Build the entity to add the child relations from the submitted form and form_state.
    rescued_form_submit_build_entity($entity_type, $entity, $form, $form_state);

    if ($entity_info['fieldable']) {
      field_attach_submit($entity_type, $entity, $form, $form_state);
    }

    // Save the entity.
    $entity->save();

    $message = $this->getOperationString('submit message', $entity_type, $entity, $operation_path);
    if (!empty($message)) {
      drupal_set_message($message);
    }

    // Redirect to the entity.
    $form_state['redirect'] = $this->getFormSubmitRedirect($this->entityType, $entity);
  }

}

/**
 * Operation handler for adding an entity: generic form.
 *
 * See RescuedEntityOperationsOperationEditGeneric for limitations of this handler.
 */
class RescuedEntityOperationsOperationAddGeneric extends RescuedEntityOperationsOperationEditGeneric {

  public $access_verb = 'create';

  /**
   * Returns strings for the operations.
   */
  function operationStrings() {
    return array(
      'tab title' => 'Add',
      'label' => 'Add',
      'description' => 'Create a new entity, with a bundle selection page if required.',
      // Note: this is ignored when on the entity form for entities with more
      // than one bundle type.
      'page title' => t('Add %entity-type'),
    ) + parent::operationStrings();
  }

  /**
   * Override getOperationStringSubstitutions().
   */
  function getOperationStringSubstitutions($entity_type, $entity, $operation_path, $parameters) {
    $substitutions = parent::getOperationStringSubstitutions($entity_type, $entity, $operation_path, $parameters);

    $clean_label = str_replace('XdrupleCommerce', '', $substitutions['%entity-type']);

    $substitutions['%entity-type'] = $clean_label;
    $substitutions['@entity-type'] = $clean_label;

    return $substitutions;
  }

  /**
   * Properties for the menu item specific to this handler.
   */
  function menu_item($operation_path, $operation_definition, $loader_position) {
    $item = array(
      // Special path which doesn't have an entity loader.
      'path' => $this->controller->path . '/add',
      // We don't want a tab.
      'type' => MENU_CALLBACK,
      // Change the callback arguments to pass NULL for the entity parameter.
      'page arguments' => array(
        'entity_operations_operation_form',
        $this->entityType,
        get_class($this),
        $operation_path,
        // Create a new empty entity.
        entity_create($this->entityType, array()),
      ),
      'access arguments' => array(
        $this->entityType,
        get_class($this),
        $operation_path,
        //$loader_position + 1,
        NULL,
      ),
    );

    // If the Entity API's admin UI is using our controller class, put this
    // menu item as a local task on the admin page.
    // @see EntityOperationsDefaultAdminUIController::menu_item_make_task()
    $ui_controller = entity_ui_controller($this->entityType);
    if (is_a($ui_controller, 'EntityOperationsDefaultAdminUIController')) {
      $ui_controller->menu_item_make_task($item);
    }

    // Add in what the parent class has.
    $item += parent::menu_item($operation_path, $operation_definition, $loader_position);
    return $item;
  }

  /**
   * Form builder for this operation.
   *
   * To get different parameters, override menu_item().
   *
   * @param $entity_type
   *  The entity type.
   * @param $entity
   *  The bundle name to add. May be NULL if the entity has no bundles or only
   *  one bundle.
   * @param $operation_path
   *  The operation path of the current operation. This is the key of the
   *  definition array in hook_entity_info().
   */
  function form($form, &$form_state, $entity_type, $entity, $operation_path) {
    // TODO: consider instead of abusing $entity, look in form state
    // build info for an extra bundle parameter?

    // Kill the cancel button.
    unset($form_state['entity_operation_form_elements']['cancel link']);

    $entity_info = entity_get_info($this->entityType);

    // Multiple cases, depending on what circumstances we're here under and
    // whether the entity has multiple bundles.
    // Case 1: we are on /add, and the entity type has either no bundles or
    // only one bundle. Just show the add form.
    if (!isset($entity_info['bundles']) || count($entity_info['bundles']) == 1) {
      // We won't have an entity here, so create one for our parent class's form.
     $values = array();

      // Set the bundle key, if we need it.
      if (count($entity_info['bundles']) == 1) {
        // Set a variable to avoid a strict warning with reset().
        $bundle_names = array_keys($entity_info['bundles']);
        $bundle_name = reset($bundle_names);
        // Must use empty(), as entity_get_info() defaults this to ''.
        if (!empty($entity_info['entity keys']['bundle'])) {
          $bundle_key = $entity_info['entity keys']['bundle'];
          $values[$bundle_key] = $bundle_name;
        }
      }

      $entity = entity_create($entity_type, $values);

      return $this->entityForm($form, $form_state, $entity_type, $entity, $operation_path);
    }

    // Case 2: we are on /add, and we don't know which bundle to use. Show a
    // list of links.
    if (empty($entity)) {
      return $this->bundleSelectionForm($form, $form_state, $entity_type, $entity, $operation_path);
    }

    // Case 3: we are on /add/BUNDLE. Show the add form.
    // We won't have an entity here, so create one for our parent class's form.
    $values = array();
    $bundle_key = $entity_info['entity keys']['bundle'];
    if (isset($entity->$bundle_key)) {
      $bundle_name = $entity->$bundle_key;
    } else {
      // TODO: Not sure why this would work.
      $bundle_name = $entity;
    }
    $values[$bundle_key] = $bundle_name;

    // Kill the setting of the title: we need more complex handling.
    unset($form_state['entity_operation_form_elements']['page title']);
    drupal_set_title(t('Add %bundle-type', array(
      '%bundle-type' => $entity_info['bundles'][$bundle_name]['label'],
    )), PASS_THROUGH);

    $entity = entity_create($entity_type, $values);

    return $this->entityForm($form, $form_state, $entity_type, $entity, $operation_path);
  }

  /**
   * Helper for form() that returns a list of bundle creation links.
   *
   * This is not really a form, just a collection of add links. But we can't
   * handle this as a page type handler because that would require returning
   * multiple menu items from a single operation handler; so we fake it.
   */
  function bundleSelectionForm($form, &$form_state, $entity_type, $entity, $operation_path) {
    $entity_info = entity_get_info($this->entityType);

    // Kill the action button.
    unset($form_state['entity_operation_form_elements']['action button']);

    // Get the base path. We will probably be on ENTITYBASE/add, though of
    // course the declaration for this operation could have set a different
    // operation path. We create links with the bundle name appended to that,
    // which will also be handled by this operation.
    $base_path = $entity_info['operations ui']['path'] . '/' . $operation_path;

    $items = array();
    foreach ($entity_info['bundles'] as $bundle_name => $bundle_info) {
      // Check access for each item.
      $verb = $this->getAccessVerb();
      $access = entity_access($verb, $this->entityType, $bundle_name);
      if ($access) {
        $items[] = l(t('Add @bundle', array('@bundle' => $bundle_info['label'])), "$base_path/$bundle_name");
      }
    }
    $form['links'][$bundle_name] = array(
      '#theme' => 'item_list',
      '#items' => $items,
    );

    return $form;
  }

}

