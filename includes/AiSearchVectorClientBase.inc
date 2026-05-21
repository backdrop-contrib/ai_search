<?php

/**
 * Base class for vector clients in Backdrop CMS.
 */
abstract class AiSearchVectorClientBase {

  /**
   * Constructor to load configurations.
   */
  public function __construct() {
    // No legacy embeddings config loading here.
  }

  /**
   * Make an HTTP request using Backdrop's built-in HTTP client.
   *
   * @param string $url
   *   The full URL to request.
   * @param string $method
   *   HTTP method (GET, POST, etc.).
   * @param array $headers
   *   Request headers.
   * @param string|null $body
   *   Request body (already encoded).
   * @param int $timeout
   *   Request timeout in seconds.
   *
   * @return object
   *   Response object with ->code, ->data, ->error properties.
   */
  protected function makeHttpRequest(string $url, string $method = 'GET', array $headers = [], ?string $body = NULL, int $timeout = 30): object {
    $options = [
      'method' => strtoupper($method),
      'headers' => $headers,
      'timeout' => $timeout,
    ];
    if ($body !== NULL) {
      $options['data'] = $body;
    }
    return backdrop_http_request($url, $options);
  }

  /**
   * Log payload data for debugging purposes.
   *
   * @param string $operation
   *   The operation being performed (e.g., 'query', 'upsert').
   * @param array  $payload
   *   The payload data to log.
   */
  protected function logPayload(string $operation, array $payload): void {
    watchdog('ai_search', "Payload for $operation: @summary", [
      '@summary' => $this->summarizeLogData($payload),
    ], WATCHDOG_DEBUG);
  }

  /**
   * Log response data for debugging purposes.
   *
   * @param string $operation
   *   The operation being performed (e.g., 'query', 'upsert').
   * @param mixed  $response
   *   The response data to log.
   */
  protected function logResponse(string $operation, $response): void {
    watchdog('ai_search', "Response from $operation: @summary", [
      '@summary' => $this->summarizeLogData($response),
    ], WATCHDOG_DEBUG);
  }

  /**
   * Summarize debug data without logging raw vectors, prompts, or responses.
   */
  protected function summarizeLogData($data): string {
    if (is_array($data)) {
      $keys = array_slice(array_keys($data), 0, 8);
      return 'array keys=' . implode(',', $keys) . '; count=' . count($data);
    }
    if (is_object($data)) {
      return 'object class=' . get_class($data);
    }
    if (is_string($data)) {
      return 'string length=' . strlen($data);
    }
    return gettype($data);
  }

  /**
   * Handle exceptions and log errors.
   *
   * @param string $operation
   *   The operation being performed (e.g., 'query', 'upsert').
   * @param \Exception $e
   *   The exception to handle.
   *
   * @throws \Exception
   *   Re-throws the exception after logging it.
   */
  protected function handleError(string $operation, \Exception $e): void {
    watchdog('ai_search', "Error during $operation: @message", [
      '@message' => $e->getMessage(),
    ], WATCHDOG_ERROR);

    throw $e;
  }

  /**
   * Abstract methods to be implemented by child classes.
   */
  abstract public function query(array $parameters);

  abstract public function upsert(array $parameters);

  abstract public function stats();

}
