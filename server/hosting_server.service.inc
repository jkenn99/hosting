<?php
/**
 * @file
 * Define the base Hosting service class.
 */

class hostingService {

  public $server;

  // The service this class represents. ie 'http', 'db'
  public $service = '';

  // The service type for this class.  ie 'mysql', 'apache'
  public $type = '';

  // The human readable name for this service type.  ie 'MySQL', 'Apache'
  public $name = '';

  // Whether or not the service is available.
  public $available = 0;

  protected $has_restart_cmd = FALSE;
  protected $has_port = FALSE;
  protected $proxyable = FALSE;

  function __construct($node, $values = NULL) {
    self::save($node);
    self::setValues($values);
  }

  /**
   * Returns human readable name for this service.
   */
  public function getName() {
    if (empty($this->name)) {
      return $this->type;
    }
    else {
      return $this->name;
    }
  }

  public function has_port() {
    return $this->has_port;
  }

  public function load() {
    $this->mergeData("SELECT port, restart_cmd, proxy_type, real_ip_from, available
                      FROM {hosting_service}
                      WHERE vid = :vid
                      AND type = :type",
                      array(
                        ':vid' => $this->server->vid,
                        ':type' => $this->type
                      )
                    );
  }

  protected function mergeData($query, $args = array()) {
    $result = db_query($query, $args)->fetchAssoc();
    $this->setValues($result);
  }

  public function setValues($record = NULL) {
    if (is_array($record)) {
      foreach ($record as $key => $value) {
        $this->{$key} = $value;
      }
    }
  }

  public function save($node) {
    if (isset($node->nid)) {
      $this->server = new stdClass();
      $this->server->nid = $node->nid;
      $this->server->vid = $node->vid;
      $this->server->title = $node->title;
    }
  }

  public function default_restart_cmd() {
    return '';
  }

  public function default_port() {
    return 0;
  }

  public function insert() {
    $id = db_insert('hosting_service')
      ->fields(array(
        'nid' => $this->server->nid,
        'vid' => $this->server->vid,
        'service' => $this->service,
        'type' => $this->type,
        'port' => $this->port,
        'restart_cmd' => isset($this->restart_cmd) ? $this->restart_cmd : '',
        'proxy_type' => $this->proxy_type,
        'real_ip_from' => $this->real_ip_from,
        'available' => $this->available,
      ))
      ->execute();
  }

  public function update() {
    $this->delete_revision();
    $this->insert();
  }

  public function delete() {
    db_delete('hosting_service')
      ->condition('service', $this->service)
      ->condition('nid', $this->server->nid)
      ->execute();
  }

  public function delete_revision() {
    db_delete('hosting_service')
      ->condition('service', $this->service)
      ->condition('vid', $this->server->vid)
      ->execute();
  }

  public function form(&$form) {
    if ($this->has_restart_cmd) {
      $form['restart_cmd'] = array(
        '#type' => 'textfield',
        '#title' => t('Restart command'),
        '#required' => !empty($this->available),
        '#description' => t('The command to run to restart this service.'),
        '#default_value' => (isset($this->restart_cmd)) ? $this->restart_cmd : $this->default_restart_cmd(),
        '#size' => 40,
        '#maxlength' => 255,
        '#weight' => -7,
      );
    }
    else {
      $form['restart_cmd'] = array(
        '#type' => 'value',
        '#value' => NULL,
      );
    }

    if ($this->has_port) {
      $form['port'] = array(
        '#type' => 'textfield',
        '#title' => t('Port'),
        '#required' => !empty($this->available),
        '#size' => 40,
        '#default_value' => isset($this->port) ? $this->port : $this->default_port(),
        '#description' => t("The port that this service is listening on."),
        '#maxlength' => 255,
        '#weight' => -8,
      );
    }
    else {
      $form['port'] = array(
        '#type' => 'value',
        '#value' => '0',
      );
    }

    if ($this->proxyable) {
      $form['proxy_type'] = array(
        '#type' => 'select',
        '#title' => t('Load balancer/reverse proxy configuration'),
        '#options' => array(
          HOSTING_SERVER_PROXY_NONE => t('Not behind a load balancer/reverse proxy'),
          HOSTING_SERVER_PROXY_XFORWARDEDFOR => t('Use X-Forwarded-For header'),
          HOSTING_SERVER_PROXY_PROXYPROTOCOL => t('Use PROXY protocol'),
        ),
        '#required' => TRUE,
        '#default_value' => isset($this->proxy_type) ? $this->proxy_type : HOSTING_SERVER_PROXY_NONE,
        '#description' => t('If this server is behind a reverse proxy or load balancer, specify the type of headers your proxy server uses to pass along the client\'s IP. Warning: If you enable PROXY protocol, connections to your nginx server will only be accepted from the load balancer'),
        '#weight' => 1,
      );
      $form['real_ip_from'] = array(
        '#type' => 'textfield',
        '#title' => t('Load balancer/reverse proxy server IP'),
        '#required' => !empty($this->proxy_type),
        '#size' => 40,
        '#default_value' => isset($this->real_ip_from) ? $this->real_ip_from : '',
        '#description' => t('Enter an IP, CIDR range, or unix: socket to accept client real IPs from.'),
        '#maxlength' => 255,
        '#weight' => 2,
      );
    }
    else {
      $form['proxy_type'] = array(
        '#type' => 'value',
        '#value' => HOSTING_SERVER_PROXY_NONE,
      );
      $form['real_ip_from'] = array(
        '#type' => 'value',
        '#value' => '',
      );
    }

  }

  public function view(&$render) {
    if ($this->has_restart_cmd) {
      $render['restart_cmd'] = array(
        '#type' => 'item',
        '#title' => t('Restart command'),
        '#markup' => filter_xss($this->restart_cmd),
      );

    }

    if ($this->has_port) {
      $render['port'] = array(
        '#type' => 'item',
        '#title' => t('Port'),
        '#markup' => filter_xss($this->port),
      );
    }
  }



  public function validate(&$node, &$form, &$form_state) {
    if ($this->has_port) {
      if ((int) $this->port <= 0) {
        form_set_error('port', t("The port you specify must be a number."));
      }
    }
  }

  public function context_options($task_type, $ref_type, &$task) {
    $task->context_options[$this->service . '_service_type'] = $this->type;

    if ($this->has_restart_cmd) {
      $task->context_options[$this->service . '_restart_cmd'] = $this->restart_cmd;
    }

    if ($this->has_port) {
      $task->context_options[$this->service . '_port'] = $this->port;
    }

    if ($this->proxyable) {
      $task->context_options[$this->service . '_proxy_type'] = $this->proxy_type;
      $task->context_options[$this->service . '_real_ip_from'] = $this->real_ip_from;
    }
  }

  public function context_import($context) {
    $this->available = 1;

    if ($this->has_port) {
      $this->port = $context->{$this->service . '_port'};
    }

    if ($this->has_restart_cmd) {
      $this->restart_cmd = $context->{$this->service . '_restart_cmd'};
    }

    if ($this->proxyable) {
      $this->proxy_type = $context->{$this->service . '_proxy_type'};
      $this->real_ip_from = $context->{$this->service . '_real_ip_from'};
    }
  }

}

