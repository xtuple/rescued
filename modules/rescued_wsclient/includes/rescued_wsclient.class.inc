<?php

/**
* @file
* Web service client Discovery Service support.
*/

/**
 * A remote endpoint type for invoking Discovery services.
 */
class WSClientDiscoveryEndpoint extends WSClientEndpoint {

  public function client() {
    // Build a custom $localConfig from WSClient settings.
    $localConfig = array();
    if (isset($this->service->settings['discovery']['application_name'])) {
      $localConfig['application_name'] = $this->service->settings['discovery']['application_name'];
    }
    if (isset($this->service->settings['discovery']['auth_class'])) {
      $localConfig['authClass'] = $this->service->settings['discovery']['auth_class'];
    }
    if (isset($this->service->settings['discovery']['oauth2_revoke_uri'])) {
      $localConfig['oauth2_revoke_uri'] = $this->service->settings['discovery']['oauth2_revoke_uri'];
    }
    if (isset($this->service->settings['discovery']['oauth2_token_uri'])) {
      $localConfig['oauth2_token_uri'] = $this->service->settings['discovery']['oauth2_token_uri'];
    }
    if (isset($this->service->settings['discovery']['oauth2_auth_url'])) {
      $localConfig['oauth2_auth_url'] = $this->service->settings['discovery']['oauth2_auth_url'];
    }
    if (isset($this->service->settings['discovery']['oauth2_federated_signon_certs_url'])) {
      $localConfig['oauth2_federated_signon_certs_url'] = $this->service->settings['discovery']['oauth2_federated_signon_certs_url'];
    }
    if (isset($this->service->settings['discovery']['client_id'])) {
      $localConfig['oauth2_client_id'] = $this->service->settings['discovery']['client_id'];
    }
    if (isset($this->service->settings['discovery']['client_secret'])) {
      $localConfig['oauth2_client_secret'] = $this->service->settings['discovery']['client_secret'];
    }
    if (isset($this->service->settings['discovery']['redirect_uri'])) {
      $localConfig['oauth2_redirect_uri'] = $this->service->settings['discovery']['redirect_uri'];
    }
    if (isset($this->service->settings['discovery']['keyfile'])) {
      $key_file = $this->service->settings['discovery']['keyfile'];
    }
    if (isset($this->service->settings['discovery']['keyfile_pass'])) {
      $keyfile_pass = $this->service->settings['discovery']['keyfile_pass'];
    }
    if (isset($this->service->settings['discovery']['iss'])) {
      $localConfig['oauth2_client_id'] = $this->service->settings['discovery']['iss'];
    }
    if (isset($this->service->settings['discovery']['scope'])) {
      $scope = $this->service->settings['discovery']['scope'];
    }
    if (isset($this->service->settings['discovery']['prn'])) {
      $prn = $this->service->settings['discovery']['prn'];
    }
    if (isset($this->service->settings['discovery']['grant_type'])) {
      $grant_type = $this->service->settings['discovery']['grant_type'];
    }

    // Once the Google library loads, this will be populated with the default settings.
    global $apiConfig;
    // Make sure the Google API PHP Client Library loads. This will overwrite $apiConfig with Google's.
    if ((!$google_lib = libraries_load('google-api-php-client')) || empty($google_lib['loaded'])) {
      watchdog('RESCUED', 'Google API PHP Client Library not found!', array(), WATCHDOG_ERROR);
    }

    // If local configuration settings are found, merge it's values with the default configuration.
    if (!empty($localConfig)) {
      $apiConfig = array_merge($apiConfig, $localConfig);
    }

    $client = new Google_Client();

    if (variable_get('rescued_debug_mode', false)) {
      // Disabling SSL cert verificaiton for local dev testing on self signed cert.
      $client::$io->setOptions(array (CURLOPT_SSL_VERIFYPEER => false));
      $client::$io->setOptions(array (CURLOPT_SSL_VERIFYHOST => false));
    }

    // Set your cached access token. Remember to replace $_SESSION with a
    // real database or memcached.
    if (isset($_SESSION['token'])) {
      $client->setAccessToken($_SESSION['token']);
    }

    // Load the key in PKCS 12 format (you need to download this from the Mobile
    // Client Admin Interface when the OAuth 2.0 service account client was created.
    $key = file_get_contents($key_file);
    // TODO: Throw an error is this doesn't load.

    // TODO: Add support for OAuth 2.0 "Web Server" Scenario Type
    $oauth = new Google_AssertionCredentials(
      $apiConfig['oauth2_client_id'],
      $scope,
      $key,
      $keyfile_pass,
      $grant_type,
      false
    );

    // Set OAuth 2.0 JWT Delegated user.
    if (isset($_SESSION['rescued'], $_SESSION['rescued']['user'])) {
      $role_user = $_SESSION['rescued']['user'];

      if (isset($_SESSION['oauth_user']) && $_SESSION['oauth_user'] !== $role_user) {
        unset($_SESSION['access_token']);
        unset($_SESSION['oauth_user']);
      }

      $oauth->prn = $role_user;
      $_SESSION['oauth_user'] = $role_user;
    } elseif (isset($prn)) {
      if (isset($_SESSION['oauth_user']) && $_SESSION['oauth_user'] !== $prn) {
        unset($_SESSION['access_token']);
        unset($_SESSION['oauth_user']);
      }
      $oauth->prn = $prn;
      $_SESSION['oauth_user'] = $prn;
    }

    $client->setAssertionCredentials($oauth);

    $config = array(
      'url' => $this->service->url,
    );

    $service = new Rescued_ApiService($client, $config);

    if (!isset($this->client)) {
      $options['exceptions'] = TRUE;
      if (!empty($this->service->settings['options'])) {
        $options += $this->service->settings['options'];
      }
      try {
        $this->client = $service;
      }
      catch (DiscoveryServiceException $e) {
        throw new WSClientException('Error initializing RESCUED client for service %name', array('%name' => $this->service->name));
      }
    }
    return $this->client;
  }

  /**
   * Retrieve metadata from the Discovery Document about available data types and operations.
   *
   * @param boolean $reset
   *   If TRUE, existing data types and operations will be overwritten.
   */
  public function initializeMetadata($reset = TRUE) {
    $service = $this->service;
    if ($this->service->settings['discovery']['debug']) {
      variable_set('rescued_debug_mode', true);
    }
    $client = $this->client();

    $discovery = $client->getDiscovery($this->service->url);

    $data_types = rescued_discovery_parse_types($discovery['schemas']);
    $operations = rescued_discovery_parse_operations($discovery['resources']);
    if ($reset) {
      $this->service->datatypes = $data_types;
      $this->service->operations = $operations;
    }
    else {
      $this->service->datatypes += $data_types;
      $this->service->operations += $operations;
    }
    $this->service->clearCache();
  }

  /**
   * Calls the RESCUED service.
   *
   * @param string $operation
   *   The name of the operation to execute.
   * @param array $arguments
   *   Arguments to pass to the service with this operation.
   */
  public function call($operation, $arguments) {
    $client = $this->client();
    try {
      $response = $client->__soapCall($operation, $arguments);
      return $response;
    }
    catch (DiscoveryServiceException $e) {
      throw new WSClientException('Error invoking the API Discovery Service %name, operation %operation: %error', array('%name' => $this->service->label, '%operation' => $operation, '%error' => $e->getMessage()));
    }
  }
}

/**
 * Exception that's used to pass information about the response when
 * a operation fails.
 */
class DiscoveryServiceException extends Exception {
  protected $response;

  public function __construct($message, $code = 0, $response = NULL, $exception = NULL) {
    parent::__construct($message, $code);
    $this->response = $response;
  }

  /**
   * Gets the response object, if any.
   */
  public function getResponse() {
    $response = $this->response;
    if (is_object($response)) {
      $response = clone $response;
    }
    return $response;
  }
}
