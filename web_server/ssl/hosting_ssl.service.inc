<?php
/**
 * @file
 * Define Hosting service class for SSL.
 */

class hostingService_http_ssl extends hostingService_http_public {

  function load() {
    parent::load();
    $this->mergeData("SELECT ssl_port, ssl_proxy_type FROM {hosting_ssl_server} WHERE vid=:vid", array(':vid' => $this->server->vid));
    $this->ssl_enabled = TRUE;
  }

  function insert() {
    parent::insert();
    $id = db_insert('hosting_ssl_server')
      ->fields(array(
        'vid' => $this->server->vid,
        'nid' => $this->server->nid,
        'ssl_port' => $this->ssl_port,
        'ssl_proxy_type' => $this->ssl_proxy_type,
      ))
      ->execute();
  }

  function delete() {
    parent::delete();
    db_delete('hosting_ssl_server')
      ->condition('nid', $this->server->nid)
      ->execute();
  }

  function delete_revision() {
    parent::delete_revision();
    db_delete('hosting_ssl_server')
      ->condition('vid', $this->server->vid)
      ->execute();
  }

  function form(&$form) {
    parent::form($form);

    $form['ssl_port'] = array(
      '#type' => 'textfield',
      '#title' => t('SSL Port'),
      '#required' => !empty($this->available),
      '#size' => 40,
      '#default_value' => isset($this->ssl_port) ? $this->ssl_port : '443',
      '#description' => t("The port that this service will use for encrypted traffic."),
      '#maxlength' => 255,
      '#weight' => -7,
    );
    if ($this->proxyable) {
      $form['ssl_proxy_type'] = array(
        '#type' => 'select',
        '#title' => t('Load balancer/proxy configuration for SSL'),
        '#options' => array(
          HOSTING_SERVER_PROXY_NONE => t('Not behind a load balancer/reverse proxy'),
          HOSTING_SERVER_PROXY_XFORWARDEDFOR => t('Use X-Forwarded-For header'),
          HOSTING_SERVER_PROXY_PROXYPROTOCOL => t('Use PROXY protocol'),
        ),
        '#required' => TRUE,
        '#default_value' => isset($this->ssl_proxy_type) ? $this->ssl_proxy_type : HOSTING_SERVER_PROXY_NONE,
        '#description' => t('Specify the load balancer/reverse proxy configuration for your SSL server.'),
        '#weight' => 1,
      );
    }
    else {
      $form['ssl_proxy_type'] = array(
        '#type' => 'value',
        '#value' => HOSTING_SERVER_PROXY_NONE,
      );
    }
  }

  function view(&$render) {
    parent::view($render);
    $render['ssl_port'] = array(
      '#type' => 'item',
      '#title' => t('SSL Port'),
      '#markup' => filter_xss($this->ssl_port),
    );
  }

  public function validate(&$node, &$form, &$form_state) {
    parent::validate($node, $form, $form_state);
    if ((int) $this->ssl_port <= 0) {
      form_set_error('ssl_port', t("The port you specify must be a number."));
    }
  }

  public function context_options($task_type, $ref_type, &$task) {
    parent::context_options($task_type, $ref_type, $task);
    $task->context_options[$this->service . '_ssl_port'] = $this->ssl_port;

    if ($this->proxyable) {
      $task->context_options[$this->service . '_ssl_proxy_type'] = $this->ssl_proxy_type;
    }
  }

  public function context_import($context) {
    parent::context_import($context);

    $this->ssl_port = $context->http_ssl_port;

    if ($this->proxyable) {
      $this->ssl_proxy_type = $context->{$this->service . '_ssl_proxy_type'};
    }
  }

}

class hostingService_http_apache_ssl extends hostingService_http_ssl {
  public $type = 'apache_ssl';
  public $name = 'Apache SSL';

  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    $command = '/usr/sbin/apachectl'; # a proper default for most of the world
    foreach (explode(':', $_SERVER['PATH']) as $path) {
      $options[] = "$path/apache2ctl";
      $options[] = "$path/apachectl";
    }
    # try to detect the apache restart command
    $options[] = '/usr/local/sbin/apachectl'; # freebsd
    $options[] = '/usr/sbin/apache2ctl'; # debian + apache2
    $options[] = $command;

    foreach ($options as $test) {
      if (is_executable($test)) {
        $command = $test;
        break;
      }
    }

    return "sudo $command graceful";
  }

}


