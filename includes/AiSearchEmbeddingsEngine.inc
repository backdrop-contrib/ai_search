<?php

/**
 * Generic embedding engine wrapper for provider/model pairs.
 */
class AiSearchEmbeddingsEngine extends AiSearchEmbeddingsBase {

  /**
   * Provider ID used to resolve the adapter.
   *
   * @var string
   */
  protected $providerId = '';

  /**
   * Constructs a new embeddings engine.
   *
   * @param array $config
   *   Engine configuration.
   * @param object $client
   *   Optional AI API wrapper.
   */
  public function __construct(array $config = [], $client = NULL) {
    if (!empty($config['provider_id'])) {
      $this->providerId = (string) $config['provider_id'];
    }
    elseif (function_exists('ai_default_provider_id')) {
      $this->providerId = ai_default_provider_id();
    }

    if (!$client && function_exists('ai_get_api')) {
      try {
        $client = ai_get_api($this->providerId);
      }
      catch (\Exception $e) {
        $client = NULL;
      }
    }

    parent::__construct($config, $client);
  }

  /**
   * Generate embeddings for the given text.
   */
  public function generateEmbeddings($text, $options = []) {
    if ($this->client && method_exists($this->client, 'embedding')) {
      return $this->client->embedding($text, $this->getModelName());
    }

    return parent::generateEmbeddings($text, $options);
  }

  /**
   * Static availability check for discovery.
   */
  public static function isAvailableStatic() {
    return function_exists('ai_get_api');
  }
}
