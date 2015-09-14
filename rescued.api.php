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
