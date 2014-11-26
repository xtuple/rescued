<?php

/**
 * Service definition for REST API.
 *
 * <p>
 * Lets you manipulate REST resource objects.
 * </p>
 *
 * <p>
 * For more information about this service, see the
 * <a href="https://developers.google.com/discovery/" target="_blank">Google APIs Discovery Service</a>
 * </p>
 */
class Rescued_ApiService extends Google_Service {
  // Debug mode to disabling SSL cert verificaiton for local dev testing on self signed cert.
  protected $debug = FALSE;

  /**
   * Constructs the internal representation of REST resource services.
   *
   * @param Google_Client $client
   */
  public function __construct(Google_Client $client, $config) {
    $this->version = 'v1alpha1';
    $this->serviceName = 'Drupal RESCUED Module';
    $this->debug = variable_get('rescued_debug_mode', FALSE);

    $client->addService($this->serviceName, $this->version);

    // Set the Google_Client property here so we can access it in Rescued_ServiceResource.
    $this->client = $client;

    $discovery = $this->getDiscovery($config['url']);

    global $apiConfig;
    $apiConfig['basePath'] = rtrim($discovery['rootUrl'], "/"); // Remove last slash from rootUrl.
    $this->servicePath = $discovery['servicePath'];

    if (!empty($discovery['resources'])) {
      // Add each service resource and it's methods.
      foreach ($discovery['resources'] as $key => $value) {
        // TODO: Make the class we use here configurable from WSClient settings
        // so you could use something like Google's Calendar class in their library.
        // The Rescued_ServiceResource class is currently specific to xTuple since
        // it uses isKey in our custom JSON-Schema.
        // Consider looking for resource method parameters where 'location' === 'path'
        // to make that more generic.
        $this->$key = new Rescued_ServiceResource($this, $this->serviceName, $key, $discovery['resources'][$key]);
      }
    }
  }

  /**
   * Get a Discovery Document for an full API or specific resource.
   *
   * @param string $resource Optional API resource name to return a single resouce.
   *
   * @return array API Discovery Document.
   */
  public function getDiscovery($url) {
    // Use drupal_static because this gets calls a lot repeatedly.
    $discovery = &drupal_static(__FUNCTION__);

    if (!isset($discovery)) {
      // Get REST API Discovery Document.
      $io = new Google_CurlIO();

      if ($this->debug) {
        // Disabling SSL cert verificaiton for local dev testing on self signed cert.
        $io->setOptions(array(CURLOPT_SSL_VERIFYPEER => FALSE));
      }

      $req = new Google_HttpRequest($url, "GET");
      $res = $io->getCachedRequest($req) ? $io->getCachedRequest($req) : $io->makeRequest($req);
      $discovery = json_decode($res->getResponseBody(), TRUE);

      // TODO: Error check for valid discovery.
    }

    return $discovery;
  }
}

class Rescued_Service extends Google_Model {
  public $id;
  public $items;
  public $kind;
  public $etag;

  public function getItems() {
    return $this->items;
  }

  public function setKind($kind) {
    $this->kind = $kind;
  }

  public function getKind() {
    return $this->kind;
  }

  public function setEtag($etag) {
    $this->etag = $etag;
  }

  public function getEtag() {
    return $this->etag;
  }

  public function setId($id) {
    $this->id = $id;
  }

  public function getId() {
    return $this->id;
  }
}

/**
 * An REST resource's collection of methods.
 * Typical usage is:
 *  <code>
 *   $service = new Rescued_ApiService(...);
 *   $result = $service->resource-name-here->get('resource-id');
 *  </code>
 */
class Rescued_ServiceResource extends Google_ServiceResource {
  private $service;

  /**
   * Mimic how the Google_ServiceResource cleans up the $postBody. This allows
   * our JSON-Patches to work below in the patch() function.
   *
   * @see Google_ServiceResource::stripNull()
   */
  protected function stripNull(&$o) {
    $o = (array) $o;
    foreach ($o as $k => $v) {
      if ($v === NULL || strstr($k, "\0*\0__")) {
        unset($o[$k]);
      }
      elseif (is_object($v) || is_array($v)) {
        $this->stripNull($o[$k]);
      }
    }
  }

  /**
   * Overridden so we can set and get the $service here.
   *
   * @see Google_ServiceResource#__construct()
   */
  public function __construct($service, $serviceName, $resourceName, $resource) {
    $this->service = $service;
    parent::__construct($service, $serviceName, $resourceName, $resource);
  }

  /**
   * Overridden so we can set and get the access token in one place.
   *
   * @param $name
   * @param $arguments
   *
   * @return Google_HttpRequest|array
   * @throws Google_Exception
   */
  public function __call($name, $arguments) {
    // TODO: Replace $_SESSION with a real database or memcached.
    if (isset($_SESSION['access_token'])) {
      $this->service->client->setAccessToken($_SESSION['access_token']);
    }

    $data = parent::__call($name, $arguments);

    // TODO: Replace $_SESSION with a real database or memcached.
    if ($this->service->client->getAccessToken()) {
      $_SESSION['access_token'] = $this->service->client->getAccessToken();
    }

    return $data;
  }

  /**
   * Adds a resource.
   *
   * @param Rescued_Service $postBody  Resource object.
   * @param array           $optParams Optional parameters.
   *
   * @return Rescued_Service Resource object.
   */
  public function insert(Rescued_Service $postBody, $optParams = array()) {
    $params = array('postBody' => $postBody);
    $params = array_merge($params, $optParams);
    $data = $this->__call('insert', array($params));
    if ($this->useObjects()) {
      return new Rescued_Service($data);
    }
    else {
      return $data;
    }
  }

  /**
   * Returns a resource.
   *
   * @param string $id           Resource key identifier.
   * @param string $resource_key Resource key name.
   * @param array  $optParams    Optional parameters.
   *
   * @return Rescued_Service Resource object
   */
  public function get($id, $resource_key, $optParams = array()) {
    $params = array($resource_key => $id);
    $params = array_merge($params, $optParams);
    $data = $this->__call('get', array($params));
    if ($this->useObjects()) {
      return new Rescued_Service($data);
    }
    else {
      return $data;
    }
  }

  /**
   * Returns list of resources.
   *
   * @param array $optParams Optional parameters.
   *
   * @return Rescued_Service Resource object
   */
  public function resourceList($optParams = array()) {
    $params = array();
    $params = array_merge($params, $optParams);
    $data = $this->__call('list', array($params));
    if ($this->useObjects()) {
      return new Rescued_Service($data);
    }
    else {
      return $data;
    }
  }

  /**
   * Updates a resource. This method supports JSON-Patch semantics.
   *
   * @param string          $id        Resource key identifier.
   * @param Rescued_Service $postBody  Resource object JSON-Patch.
   * @param array           $optParams Optional parameters.
   *
   * @return Rescued_Service Resource object
   */
  public function patch($id, $resource_key, Rescued_Service $postBody, $optParams = array()) {
    // $postBody needs to be an array or the parent::__call will strip the null
    // properties off of our JSON-Patches and screw everything up.
    // Shuffle patches off, convert to clean array and put the patches back.
    $patches = $postBody->patches;
    unset($postBody->patches);
    $this->stripNull($postBody);
    $postBody['patches'] = $patches;

    $params = array($resource_key => $id, 'postBody' => $postBody);
    $params = array_merge($params, $optParams);
    $data = $this->__call('patch', array($params));
    if ($this->useObjects()) {
      return new Rescued_Service($data);
    }
    else {
      return $data;
    }
  }

  /**
   * Deletes a resource.
   *
   * @param string $id        Resource key identifier.
   * @param array  $optParams Optional parameters.
   */
  public function delete($id, $resource_key, $optParams = array()) {
    $params = array($resource_key => $id);
    $params = array_merge($params, $optParams);
    $data = $this->__call('delete', array($params));

    return $data;
  }
}
