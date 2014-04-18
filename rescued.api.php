<?php

/**
 * @file
 * Hooks provided by the Rescued module.
 */

/**
 * Lets modules register discover client resources that they depend on. Each
 * resource will be concatenated into the discovery client URL.
 *
 * @See: rescued_default_wsclient_service()
 *
 * @param array $rescued_clients_url_resources
 *   Associative array of discovery client resources that are required by a
 *   module. In the form of: array(
 *     'client1_name_here' => array(
 *       'resource1',
 *       'resource2',
 *     ),
 *     'client2_name_here' => array(
 *       'resource1',
 *       'resource2',
 *     ),
 *   );
 *
 * @return
 *   The modified $rescued_clients_url_resources array with your
 *   additions/changes.
 */
function hook_rescued_register_clients_resources_alter(&$rescued_clients_url_resources) {
  // TODO: Modify $rescued_clients_url_resources here by changing the array
  // or adding more resource to it. e.g.
  if (!in_array('resource1', $rescued_clients_url_resources['client1_name_here'])) {
    $rescued_clients_url_resources['client1_name_here'][] = 'resource1';
  }

  return $rescued_clients_url_resources;
}

/**
 * Lets modules register that an entity property's field creation should be
 * handed off to a module that implements
 * hood_rescued_create_child_fields_alter() instead of being created by the
 * default handler.
 *
 * @See: rescued_create_child_fields()
 *
 * @param array $handled_properties
 *   Associative array of 'resource_name' => ('property_name1', 'property_name2')
 *   that is being handled by a module and should not be handled by the default
 *   child field creation handler.
 *
 * @return
 *   The modified $handled_properties array with your additions/changes.
 */
function hook_rescued_handle_create_child_fields_alter(&$handled_properties) {
  // TODO: Modify $handled_properties here by changing the
  // resource => array('properties') or adding more.
  return $handled_properties;
}

/**
 * Lets modules create fields for child relations on rescued entities.
 *
 * @See: rescued_create_child_fields()
 *
 * @param string $name
 *  Name of rescued client.
 *
 * @param string $resource
 *  Name of rescued client resource.
 *
 * @param string $property
 *  Name of resource property.
 *
 * @param string $child
 *  The child resource name.
 *
 * @param string $title
 *  The title of the property.
 *
 * @param integer $cardinality
 *  The cardinality of the property.
 *
 * @param integer $weight
 *  Field weight for sort order.
 */
function hook_rescued_create_child_fields($name, $resource, $property, $child, $title, $cardinality, $weight) {
  // TODO: Create your field and instance like this:
  if ($name === 'my_rescued_client' && $resource === 'FooResrouce' && $property === 'some-property') {
    $field_name = 'foo_' . strtolower($resource . $property);
    if (!field_info_field($field_name)) {
      $field = array(
        'field_name' => $field_name,
        'type' => 'text',
        'cardinality' => $cardinality,
        'module' => 'field',
        'storage' => array(
          'type' => 'field_sql_storage',
          'settings' => array(),
          'module' => 'field_sql_storage',
          'active' => 1,
        ),
      );

      field_create_field($field);
    }

    $entity_type = $name . '_' . strtolower($resource);
    $instance = field_info_instance($entity_type, $field_name, $entity_type);
    if (is_null($instance)) {
      $instance = array(
        'field_name' => $field_name,
        'entity_type' => $name . '_' . strtolower($resource),
        'bundle' => $name . '_' . strtolower($resource),
        'label' => $title,
      );
      $instance['widget']['weight'] = $weight;
      $instance['display']['default']['weight'] = $weight;
      field_create_instance($instance);
    }
  }
}
