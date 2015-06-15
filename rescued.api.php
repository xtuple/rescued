<?php

/**
 * @file
 * Hooks provided by the Rescued module.
 */

/**
 * Gets list of resources that a module needs.
 */
function hook_rescued_resources() {
  return [
    "Account",
    "Contact",
    "Address",
  ];
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
 * @return array
 *   The modified $handled_properties array with your additions/changes.
 */
function hook_rescued_handle_create_child_fields_alter(&$handled_properties) {
  // TODO: Modify $handled_properties here by changing the resource => array('properties') or adding more.
  return $handled_properties;
}

/**
 * Lets modules create fields for child relations on rescued entities.
 *
 * @See: rescued_create_child_fields()
 *
 * @param string  $name
 *  Name of rescued client.
 *
 * @param string  $resource
 *  Name of rescued client resource.
 *
 * @param string  $property
 *  Name of resource property.
 *
 * @param string  $child
 *  The child resource name.
 *
 * @param string  $title
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
  if ($name === 'my_rescued_client' && $resource === 'FooResource' && $property === 'some-property') {
    $field_name = 'foo_' . strtolower($resource . $property);
    if (!field_info_field($field_name)) {
      $field = [
        'field_name' => $field_name,
        'type' => 'text',
        'cardinality' => $cardinality,
        'module' => 'field',
        'storage' => [
          'type' => 'field_sql_storage',
          'settings' => [],
          'module' => 'field_sql_storage',
          'active' => 1,
        ],
      ];

      field_create_field($field);
    }

    $entity_type = $name . '_' . strtolower($resource);
    $instance = field_read_instance($entity_type, $field_name, $entity_type, [
      'include_inactive' => TRUE,
      'include_deleted' => TRUE
    ]);
    if (empty($instance)) {
      $instance = [
        'field_name' => $field_name,
        'entity_type' => $name . '_' . strtolower($resource),
        'bundle' => $name . '_' . strtolower($resource),
        'label' => $title,
      ];
      $instance['widget']['weight'] = $weight;
      $instance['display']['default']['weight'] = $weight;
      field_create_instance($instance);
    }
  }
}
