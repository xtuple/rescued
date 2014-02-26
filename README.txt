
RESTful Entity Storage Controller Unveiling Entities Dynamically Module (rescued)
---------------------------------------------------------------------------------
by Ben Thompson (bendiy), ben@xtuple.com

The RESCUED module adds support for using an APIs Discovery Service to
dynamically expose remote REST resources as entities in Drupal using the
REST API as a remote storage controller for those entities. Instead of having
to define each entity in code by hand, the RESCUED module uses the API's
Discovery Document to determine the resource's schema, permissions, methods
and parameters. The module then dynamically exposed the entities to Drupal,
uses the Entity Operations module to provide CRUD controllers and exposes
Add, View, Edit and Delete pages that allow you to interact with the remote
entities.

Installation
------------

 * RESCUED module has the follwing dependances. Download and install then from:
   - Administration menu https://drupal.org/project/admin_menu
   - Chaos tool suite https://drupal.org/project/ctools
   - Date https://drupal.org/project/date
   - Display Suite https://drupal.org/project/ds
   - EntityFieldQuery Views Backend https://drupal.org/project/efq_views
   - Entity API https://drupal.org/project/entity
   - Entity Operations https://drupal.org/project/entity_operations
   - Entity reference https://drupal.org/project/entityreference
   - Field group https://drupal.org/project/field_group
   - Inline Entity Form https://drupal.org/project/inline_entity_form
   - Libraries API https://drupal.org/project/libraries
   - Rules https://drupal.org/project/rules
   - Token https://drupal.org/project/token
   - Views https://drupal.org/project/views
   - Web service client https://drupal.org/project/wsclient
 * RESCUED module uses the following libraries. Download and install then from:
   - Google API PHP Client Library http://code.google.com/p/google-api-php-client/
   - JSON-Patch PHP Library https://github.com/bendiy/json-patch-php/tree/rfc
 * Copy the whole rescued directory to your modules directory
   (e.g. DRUPAL_ROOT/sites/all/modules) and activate the RESCUED module
 * Setup your discover service connection information at:
   admin/config/services/wsclient
 * Refresh Drupal's cache and you should see your client listed here:
   admin/structure
 * All of your remote entities can then be accessed here:
   admin/structure/{wscleint-name-here}

Learn more at http://www.xtuple.org.
