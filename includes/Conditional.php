<?php

/**
 * Represent conditional rule for an element.
 */
class Conditional {

  public $targetElementName;
  public $targetElementParents;
  public $sourceElementName;

  public $state = CONDITIONAL_STATE_REQUIRED;
  public $condition = 'value';
  public $grouping = CONDITIONAL_AND;
  public $effect = 'show';
  public $effectOptions;
  public $stateSelector = '';
  public $valueType = CONDITIONAL_DEPENDENCY_VALUES_OR;
  public $values;
  public $valueForm;
  public $value;

  protected $element;

  /**
   * Conditional constructor.
   *
   * @param array $element
   */
  public function __construct(array $element, $state, $source, $values) {
    $this->element = $element;

    $this->targetElementName = conditional_element_name($element);
    $this->targetElementParents = $element['#array_parents'];

    $this->sourceElementName = $source;
    $this->state = $state;
    $this->condition = 'value';
    $this->grouping = CONDITIONAL_AND;
    // slide or fade or show.
    $this->effect = 'slide';
    $this->effectOptions = array(
      'speed' => 300,
    );
    $this->stateSelector = '';
    $this->valueType = CONDITIONAL_DEPENDENCY_VALUES_OR;
    $this->values = is_array($values) ? $values : array($values);
  }

  public function getConditionValues() {
    $values = array();

    if ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_WIDGET) {
      $values[$this->condition] = $this->valueForm;
    }
    elseif ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_REGEX) {
      $values[$this->condition] = $this->value;
    }
    elseif ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_AND) {
      $values[$this->condition] = count($this->values) == 1
        ? $this->values[0] : $this->values;
    }
    else {
      if ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_XOR) {
        // XOR behaves like OR with added 'xor' element.
        $values[] = 'xor';
      }
      elseif ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_NOT) {
        // NOT behaves like OR with switched state.
        $this->state = strpos($this->state, '!') === 0
          ? drupal_substr($this->state, 1) : '!' . $this->state;
      }

      // OR, NOT and XOR conditions are obtained with a nested array.
      foreach ($this->values as $value) {
        $values[] = array($this->condition => $value);
      }
    }

    return $values;
  }

  /**
   * Evaluate if a dependency meets the requirements to be triggered.
   *
   * @param mixed $values
   *
   * @return bool
   *   TRUE if condition is valid, FALSE otherwise.
   */
  public function evaluate($values) {
    // Flatten array of values.
    $reference_values = array();
    foreach ((array) $values as $value) {
      // TODO: support multiple values.
      $reference_values[] = is_array($value) ? array_shift($value) : $value;
    }

    // Regular expression method.
    if ($this->valueType == CONDITIONAL_DEPENDENCY_VALUES_REGEX) {
      foreach ($reference_values as $reference_value) {
        if (!preg_match('/' . $this->values . '/', $reference_value)) {
          return FALSE;
        }
      }
      return TRUE;
    }

    switch ($this->valueType) {
      case CONDITIONAL_DEPENDENCY_VALUES_AND:
        $diff = array_diff($this->values, $reference_values);
        return empty($diff);

      case CONDITIONAL_DEPENDENCY_VALUES_OR:
        $intersect = array_intersect($this->values, $reference_values);
        return !empty($intersect);

      case CONDITIONAL_DEPENDENCY_VALUES_XOR:
        $intersect = array_intersect($this->values, $reference_values);
        return count($intersect) == 1;

      case CONDITIONAL_DEPENDENCY_VALUES_NOT:
        $intersect = array_intersect($this->values, $reference_values);
        return empty($intersect);
    }
  }

}
