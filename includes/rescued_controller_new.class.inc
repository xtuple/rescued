<?php

class RescuedControllerNew extends EntityAPIController {

  /**
   * Overridden. Need to make sure Google API PHP and JSON-Patch PHP Libraries are loaded.
   *
   * @see EntityAPIController#__construct()
   */
  public function __construct($entityType) {
    // Once the Google library loads, this will be populated with the default settings.
    global $apiConfig;
    // Make sure the Google API PHP Client Library loads. This will overwrite $apiConfig with Google's.
    if ((!$google_lib = libraries_load('google-api-php-client')) || empty($google_lib['loaded'])) {
      watchdog('RESCUED', 'Google API PHP Client Library not found!', array(), WATCHDOG_ERROR);
    }

    // Make sure the JSON-Patch PHP Library loads.
    if ((!$json_patch_lib = libraries_load('json-patch-php')) || empty($json_patch_lib['loaded'])) {
      watchdog('RESCUED', 'JSON-Patch PHP Library not found!', array(), WATCHDOG_ERROR);
    }

    parent::__construct($entityType);
  }

  /**
   * Gets entities from the static cache. Override parent function because
   * xTuple's API handles $conditions differently.
   *
   * @param $ids
   *   If not empty, return entities that match these IDs.
   * @param $conditions
   *   If set, return entities that match all of these conditions.
   *
   * @return
   *   Array of entities from the entity cache.
   */
  protected function cacheGet($ids, $conditions = array()) {
    $entities = array();
    // Load any available entities from the internal cache.
    if (!empty($this->entityCache)) {
      if ($ids) {
        $entities += array_intersect_key($this->entityCache, array_flip($ids));
      }
      // If loading entities only by conditions, fetch all available entities
      // from the cache. Entities which don't match are removed later.
      elseif ($conditions) {
        $entities = $this->entityCache;
      }
    }

    // Exclude any entities loaded from cache if they don't match $conditions.
    // This ensures the same behavior whether loading from memory or database.
    // xTuple conditions are under the 'query' key on $conditions['query'].
    if (isset($conditions['query'])) {
      foreach ($entities as $entity) {
        // Restructure conditions so array_diff_assoc() can determine matches.
        $xtuple_conditions = array();
        foreach ($conditions['query'] as $key => $condition) {
          if (array_key_exists('EQUALS', $condition)) {
            $xtuple_conditions[$key] = $condition['EQUALS'];
          } else {
            // TODO: Add support for other type of conditions?
          }
        }

        $entity_values = (array) $entity;

        if (array_diff_assoc($xtuple_conditions, $entity_values)) {
          unset($entities[$entity->{$this->idKey}]);
        }
      }
    }
    return $entities;
  }

  public function create(array $values = array()) {
    // Loop through JSON-Schema and create $values for each with the right blank type.
    $entityInfo = $this->entityInfo;
    $resource = $entityInfo['resource'];
    $schema = $entityInfo['json-schema'][$resource]['properties'];

    // Filter to just non-child fields.
    foreach ($schema as $key => $property) {
      if (isset($values[$key])) {
        continue;
      }

      if (!isset($schema[$key]['$ref']) && !isset($schema[$key]['items'])) {
        // Handle toMany key relations
        if ($property['type'] === 'string' && (isset($property['format']) ? $property['format'] !== 'date' : true)) {
// TODO: testing
          //values[$key] = "";
          $values[$key] = null;
        } elseif ($property['type'] === 'boolean') {
          $values[$key] = false;
        } else {
          $values[$key] = null;
        }
      } elseif (isset($property['$ref']) && strpos($property['$ref'], '/') !== false) {
        // Handle toOne key relations.
        $values[$key] = null;
      } elseif (isset($property['items']['$ref']) && strpos($property['items']['$ref'], '/') !== false) {
        // Handle toMany key relations.{
        $values[$key] = array();
      } else {
        $values[$key] = null;
      }
    }

    return parent::create($values);
  }

  public function load($ids = array(), $conditions = array()) {
    $entities = array();
    $key_name = $this->entityInfo['entity keys']['id'];

    // Restructure $conditions to match xTuple's query param format.
    foreach ($conditions as $con_key => $con_value) {
      if ($con_key !== 'count' &&
        $con_key !== 'q' &&
        $con_key !== 'query' &&
        $con_key !== 'maxResults' &&
        $con_key !== 'pageToken' &&
        $con_key !== 'rowLimit') {

        $conditions['query'][$con_key] = array(
          'EQUALS' => $con_value,
        );
        unset($conditions[$con_key]);
      }
    }

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($ids) ? array_flip($ids) : FALSE;

    // Try to load entities from the static cache.
    if ($this->cache) {
      $entities = $this->cacheGet($ids, $conditions);

      // Make sure this is a fully loaded entity.
      foreach ($entities as $cached_id => $cached_entity) {
        if (!isset($cached_entity->eTag)) {
          unset($entities[$cached_id]);
        }
      }

      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Support the entitycache module if activated.
    if (!empty($this->entityInfo['entity cache']) && $ids && !$conditions) {
      $cached_entities = EntityCacheControllerHelper::entityCacheGet($this, $ids, $conditions);

      // Make sure this is a fully loaded entity.
      foreach ($cached_entities as $entitycached_id => $entitycached_entity) {
        if (!isset($entitycached_entity->eTag)) {
          unset($cached_entities[$entitycached_id]);
        }
      }

      // If any entities were loaded, remove them from the ids still to load.
      $ids = array_diff($ids, array_keys($cached_entities));
      $entities += $cached_entities;

      // Add loaded entities to the static cache.
      if ($this->cache && !empty($cached_entities)) {
        $this->cacheSet($cached_entities);
      }
    }

    // If $ids is empty, assume list loading.
    if ($ids === FALSE || ($conditions && !$passed_ids)) {
      $resources = rescued_api_resource_list($this->entityInfo['discovery-client'], $this->entityInfo['resource'], $conditions);

      foreach($resources as $resource) {
        if (isset($this->entityInfo['entity class']) && $class = $this->entityInfo['entity class']) {
          // Instantiate a new object of this class passing the $resource values in to be assigned.
          $entity = new $class($resource, $this->entityType);

          // Set the original_entity so we can do a JSON-Patch diff after changes.
          $entity->originalEntity = $resource;
        }

        // TODO: Heard some rumors that the array key here has to be an integer for fields API to work.
        $queried_entities[$entity->$key_name] = $entity;
      }
    } elseif ($passed_ids) {
      // This method takes an array of IDs, but our webservice only supports loading one entity at a time.
      foreach ($ids as $id) {
        // This function should contain all the code to make a request to the web service and handle any errors.
        if ($resource = rescued_api_resource_load($this->entityInfo['discovery-client'], $this->entityInfo['resource'], $id, $key_name)) {
          if (isset($this->entityInfo['entity class']) && $class = $this->entityInfo['entity class']) {
            // Instantiate a new object of this class passing the $resource values in to be assigned.
            $entity = new $class($resource['data'], $this->entityType);

            // Set the original_entity so we can do a JSON-Patch diff after changes.
            $entity->originalEntity = $resource['data'];
            $entity->eTag = $resource['etag'];
          }

          // TODO: Heard some rumors that the array key here has to be an integer for fields API to work.
          $queried_entities[$entity->$key_name] = $entity;
        }
      }
    }

    // Pass all entities loaded from the REST API through $this->attachLoad(),
    // which attaches fields (if supported by the entity type) and calls the
    // entity type specific load callback, for example hook_node_load().
    if (!empty($queried_entities)) {
      $this->attachLoad($queried_entities);
      $entities += $queried_entities;
    }

    if ($this->cache) {
      // Add entities to the cache.
      if (!empty($queried_entities)) {
        $cacheable = false;
        foreach ($queried_entities as $etag_entity) {
          if (!isset($etag_entity->eTag)) {
            // If any entities are missing an etag, it's not the full entity and we do not want to cache any of these.
            // xTuple list requests do not inclued etags, so if one is missing, all will be missing.
            $cacheable = false;
            break;
          } else {
            $cacheable = true;
            // Assume if one has eTag, all of them should.
            break;
          }
        }

        if ($cacheable) {
          $this->cacheSet($queried_entities);

          // Entitycache module support: Add entities to the entity cache if we are
          // not loading a revision.
          if (!empty($this->entityInfo['entity cache']) && !empty($queried_entities)) {
            EntityCacheControllerHelper::entityCacheSet($this, $queried_entities);
          }

          // Remember if we have cached all entities now.
          if (!$conditions && $ids === FALSE) {
            $this->cacheComplete = TRUE;
          }
        }
      }
    }
    // Ensure that the returned array is ordered the same as the original
    // $ids array if this was passed in and remove any invalid ids.
    if ($passed_ids && $passed_ids = array_intersect_key($passed_ids, $entities)) {
      foreach ($passed_ids as $id => $value) {
        $passed_ids[$id] = $entities[$id];
      }
      $entities = $passed_ids;
    }

    return $entities;
  }

  public function delete($ids) {
    // TODO: Do we really want to load these first?
    $entities = $ids ? $this->load($ids) : FALSE;

    if (!$entities) {
      // Do nothing, in case invalid or no ids have been passed.
      return;
    }
    $ids = array_keys($entities);

    $key_name = $this->entityInfo['entity keys']['id'];

    // This method takes an array of IDs, but our webservice only supports deleting one entity at a time.
    foreach ($ids as $id) {
      // This function should contain all the code to make a request to the web service and handle any errors.
      rescued_api_resource_delete($this->entityInfo['discovery-client'], $this->entityInfo['resource'], $id, $key_name);
    }
  }

  public function save($entity) {
    $entityInfo = $entity->entityInfo();
    $resource = $entityInfo['resource'];
    $schema = $entityInfo['json-schema'][$resource]['properties'];

    // Load the stored entity, if any.
    if (!empty($entity->{$this->idKey}) && !isset($entity->original) && (!isset($entity->is_new) || $entity->is_new === false)) {
      // In order to properly work in case of name changes, load the original
      // entity using the id key if it is available.

      // TODO: This might be able to just use $entity->originalEntity cast as
      // an object to avoid another REST call, but that needs to go through
      // xdfields_rest_storage_field_storage_load() to get the field converted...
      //$class = $this->entityInfo['entity class'];
      //$entity->original = new $class($entity->originalEntity, $this->entityType);

      $entity->original = entity_load_unchanged($this->entityType, $entity->{$this->idKey});
    }

    $this->invoke('presave', $entity);

    $entity->is_new = !empty($entity->is_new) || empty($entity->{$this->idKey});

    // We do things a little different by calling the field invoke hooks before
    // saving the entity. This is because 'xdfields_rest_storage' are saved
    // WITH their entity, not as seperate tables.
    // @See: _xdfields_rest_storage_field_format()
    if (!$entity->is_new) {
      $original_entity = $entity->originalEntity;
      $this->resetCache(array($entity->{$this->idKey}));
      $this->invoke('update', $entity);
    } else {
      // The above $this->invoice('presave', $entity) will set a UUID on
      // xTuple's nkey through uuid_entity_presave(). Some times the nkey is
      // not a UUID field and xTuple's API takes care of setting it, so we want
      // to reset that back to null.
      if (isset($entityInfo['uuid']) && $entityInfo['uuid'] == TRUE && !empty($entityInfo['entity keys']['uuid'])) {
        $uuid_key = $entityInfo['entity keys']['uuid'];
        if (!empty($entity->{$uuid_key}) && preg_match('/^[a-zA-Z0-9]{8}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{12}?$/', $entity->{$uuid_key})) {
          $entity->{$uuid_key} = null;
        }
      }

      $this->invoke('insert', $entity);
    }


    // Build up a clean array of just the RESCUED resource properties from the entity.
    $new_entity = array();
    $empty_entity = TRUE;
    foreach ($schema as $key => $value) {
      if (isset($entity->$key)) {
        if (empty($entity->$key) && $entity->is_new && !is_bool($entity->$key)) {
          $new_entity[$key] = null;
        } else {
          if ($entity->is_new) {
            $empty_entity = FALSE;
          }
          $new_entity[$key] = $entity->$key;
        }
      }
    }

    // TODO: Don't need this yet, but the Google lib expects it.
    $optParams = array();

    // Convert child objects to just their keys.
    foreach ($schema as $key => $value) {
      if (!isset($entity->$key)) {
        continue;
      }
      if (is_array($entity->{$key}) && array_key_exists(LANGUAGE_NONE, $entity->{$key})) {
        // This property is loaded through fields API. Skip it.

        // TODO: Need a better way to identify these properties.
        $foo = true;
        $new_entity[$key] = $original_entity[$key];
      } elseif (isset($value['$ref'], $new_entity[$key]) && strpos($value['$ref'], '/') === false) {
        $relation_resource = $value['$ref'];
        $relation_entityType = strtolower($relation_resource);
        $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
        $child_entityInfo = entity_get_info($child_entityType);
        $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];
        $relation_key = rescued_json_schema_get_resource_key($relation_schema);

        if (!isset($child_entityInfo['is_nested_only'])) {
//           if (isset($new_entity[$key][$relation_key])) {
//             $new_key_value = $new_entity[$key][$relation_key];
//           } else {
//             $new_key_value = null;
//           }

//           $new_entity[$key] = $new_key_value;
          if (!$entity->is_new) {
            if (isset($original_entity[$key][$relation_key])) {
              $org_key_value = $original_entity[$key][$relation_key];
            } else {
              $org_key_value = null;
            }
            $original_entity[$key] = $org_key_value;
          }
        }
      } elseif (isset($value['items']['$ref'], $new_entity[$key]) && strpos($value['items']['$ref'], '/') === false) {
        $relation_resource = $value['items']['$ref'];
        $relation_entityType = strtolower($relation_resource);
        $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
        $child_entityInfo = entity_get_info($child_entityType);
        $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];
        $relation_key = rescued_json_schema_get_resource_key($relation_schema);

        foreach ($new_entity[$key] as $child_key => $child_value) {
          if (!isset($child_entityInfo['is_nested_only'])) {
            $new_entity[$key][$child_key] = $new_entity[$key][$child_key][$relation_key];
            if (!$entity->is_new && isset($original_entity[$key][$child_key], $original_entity[$key][$child_key][$relation_key])) {
              $original_entity[$key][$child_key] = $original_entity[$key][$child_key][$relation_key];
            }
          }
        }
      }
    }

    if ($entity->is_new) {
      // TODO: Check if this entity is populated.
      if ($empty_entity) {
        // Nothing to save.
        return false;
      }

// TODO: Remove this, testing new user reg.
      if ($this->entityType === 'xtuple_xdrupleusercontact') {
        // This is a hack because the field is saved before the user is added.
        // Need to find a hook for after the user is added to get the field association
        // and check if the id is the correct uid, if not, update the field
        // and the xtuple_xdrupleusercontact with the correct uid.
        //$new_entity['uid'] = db_query('SELECT MAX(uid) FROM {users}')->fetchField() + 1;
      }

      // Create a service object that will be the body of the request.
      $requestBody = new Rescued_Service($new_entity);

      $return = rescued_api_resource_insert($this->entityInfo['discovery-client'], $this->entityInfo['resource'], $requestBody, $optParams);

      // TODO: In theory, we could take returned patches and apply them to the
      // entity and set a cache for the new id. Then when the "view" page is
      // loaded, it could just get the entity from the cache.
      if (empty($entity->{$entityInfo['entity keys']['id']}) && !empty($return['id'])) {
        $entity->{$entityInfo['entity keys']['id']} = $return['id'];
      }
    }
    else {
      $new_json = json_encode($new_entity);
      $original_json = json_encode($original_entity);

      // A PATCH request body expects an array key of 'patches' with patch objects.
      $postBody = array(
        'patches' => JsonPatch::diff($original_json, $new_json),
      );

      // For debugging.
      $patch = json_encode($postBody['patches']);

      if (count($postBody['patches']) > 0) {
        // Create a service object that will be the body of the request.
        $requestBody = new Rescued_Service($postBody);

        // Set the etag from original load of the entity.
        $requestBody->setEtag($entity->eTag);

        // Get the id of the originalEntity so we can edit it.
        $id = $entity->originalEntity[$this->idKey];
        $key_name = $this->entityInfo['entity keys']['id'];

        // This function should contain all the code to make a request to the web service and handle any errors.
        $return = rescued_api_resource_patch($this->entityInfo['discovery-client'], $this->entityInfo['resource'], $id, $key_name, $requestBody, $optParams);

        // TODO: In theory, we could take returned patches and apply them to the
        // entity and update the cache. Then when the "view" page is loaded, it
        // could just get the entity from the cache.

        // Update the etag to the latest version.
        $entity->eTag = $return['etag'];
      } else {
        $id = $entity->{$this->idKey};
        $return = array('id' => $id);
      }

      unset($entity->is_new);
      unset($entity->is_new_revision);
      unset($entity->original);

      // Run through attachLoad() to reformat the entity from xTuple's structure to Drupal's.
      // This will return that attached xdfields_rest_storage fields to the proper format.
      $entities = array($id => $entity);
      $this->attachLoad($entities);

      return $return;
    }
  }

  public function buildContent($entity, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $content = array();

    $resource_schema = $this->entityInfo['json-schema'][$this->entityInfo['resource']]['properties'];
    $parent = strtolower($this->entityInfo['label']);

    // Allow modules to modify the structured entity.
    $context = array(
      'entity_type' => $this->entityType,
      'langcode' => $langcode,
      'view_mode' => $resource_schema,
    );
    drupal_alter('rescued_entity_build_content_view', $entity, $resource_schema, $context);

    $groups = array();
    foreach ($resource_schema as $key => $properties) {
      $content = array_merge($content, $this->build_content_recursively($entity, $key, $properties));
    }

    return parent::buildContent($entity, $view_mode, $langcode, $content);
  }

  private function build_content_recursively($entity, $key, $properties) {
    $content = array();

    if (isset($entity->{$key}) && is_array($entity->{$key}) && array_key_exists(LANGUAGE_NONE, $entity->{$key})) {
      // This property is loaded through fields API. Skip it.

      // TODO: Need a better way to identify these properties.
      $foo = true;
    } elseif (isset($properties['$ref']) && strpos($properties['$ref'], '/') === false) {
      // Handle toOne relations.
      $content[$key] = array(
        '#theme' => 'field',
        '#weight' => 20,
        '#title' => t($properties['title']),
        '#label_display' => 'hidden', // Hiding this because it's in the field group header.
        '#access' => TRUE,
        '#bundle' => $this->entityType, // Need this so Display Suite module doesn't complain.
        '#view_mode' => 'full',
        '#language' => LANGUAGE_NONE,
        '#field_name' => $key,
        '#field_type' => 'entityreference',
        '#entity_type' => $this->entityType,
        '#formatter' => 'entityreference_entity_view',
        '#prefix' => '<div class="field-group-container clearfix">',
        '#suffix' => '</div>',
        '#items' => array(),
      );

      // We don't need to build this if it's null.
      if(!property_exists($entity, $key) || is_null($entity->{$key})) {
        return $content;
      }

      $entityInfo = $entity->entityInfo();
      $relation_resource = $properties['$ref'];
      $relation_entityType = strtolower($relation_resource);
      $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
      $child_entityInfo = entity_get_info($child_entityType);
      $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];
      $class = $child_entityInfo['entity class'];
      $child_data = json_decode(json_encode($entity->{$key}), true);

      if (!is_array($child_data) && is_string($child_data)) {
        $child_data = array($child_entityInfo['entity keys']['id'] => $child_data);
      }
      $child_entity = new $class($child_data, $child_entityType);
      $child_content = $child_entity->buildContent('full', NULL, NULL);

      $content[$key]['#object'] = $child_entity;
      $content[$key]['#items'] = array(
        array(
          'target_id' => $child_entity->{$child_entityInfo['entity keys']['id']},
          'entity' => json_decode(json_encode($child_entity), FALSE),
          '#access' => TRUE,
        ),
      );
      $content[$key][] = array();

      $content[$key][0][$child_entityType][$child_entity->{$child_entityInfo['entity keys']['id']}] = $child_content;
    } elseif ((isset($properties['items']) && isset($properties['items']['$ref']) &&
        strpos($properties['items']['$ref'], '/') === false )) {
      // Handle toMany relations.
      $content[$key] = array(
        '#theme' => 'field',
        '#weight' => 30,
        '#title' => t($properties['title']),
        '#label_display' => 'hidden', // Hiding this because it's in the field group header.
        '#access' => TRUE,
        '#bundle' => $this->entityType, // Need this so Display Suite module doesn't complain.
        '#view_mode' => 'full',
        '#language' => LANGUAGE_NONE,
        '#field_name' => $key,
        '#field_type' => 'fieldset',
        '#entity_type' => $this->entityType,
        '#formatter' => 'text_default',
        '#prefix' => '<div class="field-group-container clearfix">',
        '#suffix' => '</div>',
        '#items' => array(),
      );

      // We don't need to build this if it's null.
      if(!property_exists($entity, $key) || is_null($entity->{$key})) {
        return $content;
      }

      $entityInfo = $entity->entityInfo();
      $relation_resource = $properties['items']['$ref'];
      $relation_entityType = strtolower($relation_resource);
      $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
      $child_entityInfo = entity_get_info($child_entityType);
      $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];

      foreach ($entity->{$key} as $list_key => $list_item) {
        $class = $child_entityInfo['entity class'];
        $child_data = json_decode(json_encode($list_item), true);

        if (!is_array($child_data) && is_string($child_data)) {
          $child_data = array($child_entityInfo['entity keys']['id'] => $child_data);
        }
        $child_entity = new $class($child_data, $child_entityType);
        $child_content = $child_entity->buildContent('full', NULL, NULL);

        $content[$key]['#items'][$list_key] = $entity->{$key}[$list_key];
        $content[$key]['#items'][$list_key]['#access'] = TRUE;
        $content[$key][$list_key] = $child_content;
      }

    } elseif (isset($properties['items']) && isset($properties['items']['$ref']) &&
      strpos($properties['items']['$ref'], '/') !== false ) {
      // Handle toMany key relations.
      $relation_resource = explode("/", $properties['items']['$ref']);
      $relation_key = $relation_resource[1];

      // Trim 'Relations' off the key.
      $relation_resource = $relation_resource[0];
      // TODO: Consider $child_entityType instead of $relation_resource in the Ajax callback url.
      // $entityInfo = $entity->entityInfo();
      // $relation_entityType = strtolower($relation_resource);
      // $child_entityType = $entityInfo['discovery-client'] . '_' . $relation_entityType;
      // $child_entityInfo = entity_get_info($child_entityType);
      // $relation_schema = $child_entityInfo['json-schema'][$relation_resource]['properties'];

      $content[$key] = array(
        '#theme' => 'field',
        '#weight' => 10,
        '#title' => t($properties['title']),
        '#label_display' => 'above', // Hiding this because it's in the field group header.
        '#access' => TRUE,
        '#bundle' => $this->entityType, // Need this so Display Suite module doesn't complain.
        '#view_mode' => 'full',
        '#language' => LANGUAGE_NONE,
        '#field_name' => $key,
        '#field_type' => 'textfield',
        '#entity_type' => $this->entityType,
        '#formatter' => 'text_default',
        '#items' => array(),
      );

      foreach ($entity->{$key} as $list_key => $list_item) {
        $list_object = json_decode(json_encode($list_item), FALSE);
        $content[$key]['#items'][$list_key] = $entity->{$key}[$list_key];

        $content[$key][$list_key] = array(
          '#type' => 'link',
          '#title' => $entity->{$key}[$list_key],
          '#href' => $this->entityInfo['discovery-client'] . '/' . $relation_resource . '/' . $entity->{$key}[$list_key],
          '#options' => array(
            'entity_type' => $this->entityInfo['discovery-client'] . '_' . $relation_resource,
            'entity' => $list_object,
            'attributes' => array(
              'typeof' => array('skos:Concept'), // TODO: Not sure about this.
              'property' => array('rdfs:label', 'skos:prefLabel'), // TODO: Not sure about this.
              'datatype' => '',
            ),
          ),
        );
      }
    } else {
      // Handle basic property.
      $content[$key] = array(
        '#theme' => 'field',
        '#weight' => 0,
        '#title' =>t($properties['title']),
        '#label_display' => 'above',
        '#access' => TRUE,
        '#bundle' => $this->entityType, // Need this so Display Suite module doesn't complain.
        '#view_mode' => 'full',
        '#language' => LANGUAGE_NONE,
        '#field_name' => $key,
        '#field_type' => rescued_json_schema_to_form_type($properties['type']),
        '#entity_type' => $this->entityType,
        '#items' => array(array('value' => (!property_exists($entity, $key) || is_array($entity->{$key})) ? '' : $entity->{$key})),
        '#formatter' => 'text_default',
        0 => array('#markup' => check_plain((!property_exists($entity, $key) || is_array($entity->{$key})) ? '' : $entity->{$key}))
      );
    }

    return $content;
  }
}

/**
 * Rescued Entity class.
 */
class RescuedEntityNew extends Entity {

  /**
   * Defines the entity label when the 'entity_class_label' callback is used in
   * hook_entity_info().
   */
  protected function defaultLabel() {
    $label = parent::defaultLabel();

    $entityInfo = $this->entityInfo();

    $clean_label = str_replace('XdrupleCommerce', '', $entityInfo['label']);

    // Allow modules to modify the structured entity.
    $context = array(
      'entity_type' => $this->entityType,
    );
    drupal_alter('rescued_entity_default_label', $this, $label, $context);

    // This will return something like "Contact 42" as the label.
    return $clean_label . ': ' . $label;
  }
}
