<?php

/**
 * Utility for chunking text in an NLP-friendly manner.
 */
class AITextChunker {

  /**
   * Rough average characters-per-token ratio for English text, used to
   * approximate token counts without a model-specific BPE tokenizer.
   */
  const CHARS_PER_TOKEN = 4;

  /**
   * Generate chunks from a string, sized by an approximate token count.
   *
   * chunkText() operates on characters. Embedding models size their context
   * windows in tokens, so callers configuring "chunk size" in tokens need
   * that value converted to characters first — otherwise a "500 token"
   * setting produces chunks ~4x smaller than intended.
   *
   * @param string $text
   *   The text to chunk.
   * @param int $maxTokens
   *   The approximate maximum tokens per chunk.
   * @param int $minOverlapTokens
   *   The approximate minimum token overlap between chunks.
   *
   * @return array
   *   An array of text chunks.
   */
  public static function chunkTextByTokens($text, $maxTokens, $minOverlapTokens) {
    return self::chunkText(
      $text,
      max(1, $maxTokens) * self::CHARS_PER_TOKEN,
      max(0, $minOverlapTokens) * self::CHARS_PER_TOKEN
    );
  }

  /**
   * Generate chunks from a string.
   *
   * @param string $text
   *   The text to chunk.
   * @param int $maxSize
   *   The maximum size of the chunk (multibyte safe).
   * @param int $minOverlap
   *   The minimum overlap between chunks (multibyte safe).
   *
   * @return array
   *   An array of text chunks.
   */
  public static function chunkText($text, $maxSize, $minOverlap) {
    $remainingText = $text;
    $chunks = [];

    while ($remainingText) {
      $length = mb_strlen($remainingText);
      if ($length <= $maxSize) {
        $chunks[] = $remainingText;
        break;
      }

      // Calculate the offset to achieve the max position.
      $offset = $maxSize - $length;

      // Get a chunk ending with whitespace that is less than our chunk size.
      $chunk = mb_substr(
        $remainingText,
        0,
        // Length will be the position of the last whitespace within the max
        // chunk size, or otherwise the max chunk size itself.
        self::getLastWhitespacePosition($remainingText, $offset) ?: $maxSize
      );
      $chunks[] = trim($chunk);

      // Remove our chunk, keeping our minimum overlap, again breaking at whitespace.
      // If $chunk has no whitespace to break on (e.g. a long unbroken token), fall
      // back to advancing past the whole chunk minus overlap so we always make
      // forward progress instead of looping on the same text forever.
      $remainingText = mb_substr(
        $remainingText,
        self::getLastWhitespacePosition($chunk, -$minOverlap) ?: max(mb_strlen($chunk) - $minOverlap, 1)
      );
    }

    // Remove any empty chunks.
    return array_filter($chunks);
  }

  /**
   * Get the position of the last whitespace character with multibyte safety.
   *
   * @param string $haystack
   *   The text to search in.
   * @param int $offset
   *   The offset to search from.
   *
   * @return int|null
   *   The multibyte safe position of the last whitespace character or NULL if
   *   there is no whitespace.
   */
  public static function getLastWhitespacePosition($haystack, $offset) {
    $whitespacePosition = max(
      mb_strrpos($haystack, " ", $offset),
      mb_strrpos($haystack, "\n", $offset),
      mb_strrpos($haystack, "\r", $offset),
      mb_strrpos($haystack, "\t", $offset)
    );

    return ($whitespacePosition !== false) ? $whitespacePosition : NULL;
  }
}
