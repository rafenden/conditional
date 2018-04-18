<?php

/**
 * Builds a list of special fields handlers to be executed when building the
 * #states array. The keys are handler function names and the key/value pairs
 * are field properties and their values that trigger the execution of the handler.
 *
 * The handlers themselves must accept the parameters $field, $field_info,
 * $options and $state.
 *
 * @see conditional_field_attach_form()
 */
function conditional_states_handlers() {
  $handlers = array(
    'conditional_states_handler_select_multiple' => array(
      '#type' => 'select',
      '#multiple' => TRUE,
    ),
    'conditional_states_handler_checkbox' => array(
      '#type' => 'checkbox',
    ),
    'conditional_states_handler_checkboxes' => array(
      '#type' => 'checkboxes',
    ),
    'conditional_states_handler_textfield' => array(
      '#type' => 'textfield',
    ),
    'conditional_states_handler_textarea' => array(
      '#type' => 'textarea',
    ),
    'conditional_states_handler_date_combo' => array(
      '#type' => 'date_combo',
    ),
    'conditional_states_handler_link_field' => array(
      '#type' => 'link_field',
    ),
    'conditional_states_handler_link_addressfield' => array(
      '#addressfield' => 1,
    ),
  );

  // Allow other modules to modify the handlers
  drupal_alter('conditional_states_handlers', $handlers);

  return $handlers;
}

/**
 * States handler for multiple select lists.
 *
 * Multiple select fields always require an array as value.
 * In addition, since our modified States API triggers a dependency only if all
 * reference values of type Array are selected, a different selector must be
 * added for each value of a set for OR, XOR and NOT evaluations.
 */
function conditional_states_handler_select_multiple($field, $field_info, Conditional $conditional, &$state) {
  switch ($conditional->valueType) {
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET:
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_AND:
      $state[$conditional->state][$conditional->stateSelector]['value'] = (array) $state[$conditional->state][$conditional->stateSelector]['value'];
      return;

    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_XOR:
      $select_states[$conditional->state][] = 'xor';

    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX:
      $regex = TRUE;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_NOT:
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_OR:
      foreach ($conditional->values as $value) {
        $select_states[$conditional->state][] = array(
          $conditional->stateSelector => array(
            $conditional->condition => empty($regex) ? array($value) : $conditional->value,
          ),
        );
      }
      break;
  }

  $state = $select_states;
}

/**
 * States handler for single on/off checkbox.
 */
function conditional_states_handler_checkbox($field, $field_info, Conditional $conditional, &$state) {
  switch ($conditional->valueType) {
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET:
      $checked = $conditional->values == $field['#on_value'];
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX:
      $checked = preg_match('/' . $conditional->values . '/', $field['#on_value']);
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_AND:
      // ANDing values of a single checkbox doesn't make sense: just use the first value.
      $checked = $conditional->values == $field['#on_value'];
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_XOR:
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_OR:
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_NOT:
      $checked = in_array($field['#on_value'], $conditional->values) ? TRUE : FALSE;
      break;
  }

  $state[$conditional->state][$conditional->stateSelector] = array('checked' => $checked);
}

/**
 * States handler for checkboxes.
 */
function conditional_states_handler_checkboxes($field, $field_info, Conditional $conditional, &$state) {
  // Checkboxes are actually different form fields, so the #states property
  // has to include a state for each checkbox.
  $checkboxes_selectors = array();

  switch ($conditional->valueType) {
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET:
      foreach ($conditional->value as $value) {
        $checkboxes_selectors[conditional_field_selector($field[current($value)])] = array('checked' => TRUE);
      }
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX:
      // We interpret this as: checkboxes whose values match the regular
      // expression should be checked.
      foreach ($field['#options'] as $key => $label) {
        if (preg_match('/' . $conditional->values . '/', $key)) {
          $checkboxes_selectors[conditional_field_selector($field[$key])] = array('checked' => TRUE);
        }
      }
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_AND:
      foreach ($conditional->values as $value) {
        $checkboxes_selectors[conditional_field_selector($field[$value])] = array('checked' => TRUE);
      }
      break;
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_XOR:
      $checkboxes_selectors[] = 'xor';
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_OR:
    case CONDITIONAL_FIELDS_DEPENDENCY_VALUES_NOT:
      foreach ($conditional->values as $value) {
        $checkboxes_selectors[] = array(conditional_field_selector($field[$value]) => array('checked' => TRUE));
      }
      break;
  }

  $state = array($conditional->state => $checkboxes_selectors);
}

/**
 * States handler for text fields.
 */
function conditional_states_handler_textfield($field, $field_info, Conditional $conditional, &$state) {
  // Text fields values are keyed by cardinality, so we have to flatten them.
  // TODO: support multiple values.
  if ($conditional->valueType == CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET) {
    // Cast as array to handle the exception of autocomplete text fields.
    $value = (array) $state[$conditional->state][$conditional->stateSelector]['value'][0];
    $state[$conditional->state][$conditional->stateSelector]['value'] = array_shift($value);
  }
}

/**
 * States handler for text areas.
 */
function conditional_states_handler_textarea($field, $field_info, Conditional $conditional, &$state) {
  conditional_states_handler_textfield($field, $field_info, $conditional, $state);
}

/**
 * States handler for date combos.
 */
function conditional_states_handler_date_combo($field, $field_info, Conditional $conditional, &$state) {
  // Date text.
  if ($field_info['instance']['widget']['type'] == 'date_text') {
    if ($conditional->valueType == CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET) {
      $state[$conditional->state][$conditional->stateSelector]['value'] = $state[$conditional->state][$conditional->stateSelector]['value'][0]['value']['date'];
    }
    return;
  }

  // Add a condition for each date part.
  $date_selectors = array();

  $regex = $conditional->valueType == CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX;

  // Date popup.
  if ($field_info['instance']['widget']['type'] == 'date_popup') {
    $date_selectors[conditional_field_selector($field['value']['date'])] = array(
      'value' => $regex ? $conditional->value : $conditional->valueForm[0]['value']['date'],
    );

    if ($field_info['field']['settings']['granularity']['hour'] || $field_info['field']['settings']['granularity']['minute'] || $field_info['field']['settings']['granularity']['second']) {
      $date_selectors[conditional_field_selector($field['value']['time'])] = array(
        'value' => $regex ? $conditional->value : $conditional->valueForm[0]['value']['time'],
      );
    }
  }
  // Date select.
  else {
    foreach ($field_info['field']['settings']['granularity'] as $date_part) {
      if ($date_part) {
        $date_selectors[conditional_field_selector($field['value'][$date_part])] = array(
          'value' => $regex ? $conditional->value : $conditional->valueForm[0]['value'][$date_part],
        );
      }
    }
  }

  $state = array($conditional->state => $date_selectors);
}

/**
 * States handler for links provided by the Link module.
 */
function conditional_states_handler_link_field($field, $field_info, Conditional $conditional, &$state) {
  $link_selectors = array();
  $regex = $conditional->valueType == CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX;

  // Add a condition for each link part (Title and URL)
  if ($field_info['instance']['settings']['title'] == 'optional' || $field_info['instance']['settings']['title'] == 'required') {
    $link_selectors[conditional_field_selector($field['title'])] = array('value' => $regex ? $conditional->value : $conditional->valueForm[0]['title']);
  }
  $link_selectors[conditional_field_selector($field['url'])] = array('value' => $regex ? $conditional->value : $conditional->valueForm[0]['url']);

  $state = array($conditional->state => $link_selectors);
}

/**
 * States handler for links provided by the Addressfield module.
 */
function conditional_states_handler_link_addressfield($field, $field_info, Conditional $conditional, &$state) {
  if ($conditional->valueType != CONDITIONAL_FIELDS_DEPENDENCY_VALUES_WIDGET) {
    return;
  }

  $regex = $conditional->valueType == CONDITIONAL_FIELDS_DEPENDENCY_VALUES_REGEX;
  $keys = array();

  if ($field['#handlers']['address']) {
    $keys += array('country', 'thoroughfare', 'premise', 'postal_code', 'locality', 'administrative_area');
  }

  if ($field['#handlers']['organisation']) {
    $keys += array('organisation_name');
  }

  if ($field['#handlers']['name-oneline']) {
    $keys += array('name_line');
  }
  elseif ($field['#handlers']['name-full']) {
    $keys += array('first_name', 'last_name');
  }

  $addressfield_selectors = array();

  foreach ($keys as $key) {
    $addressfield_selectors[str_replace('%key', $key, $conditional->stateSelector)] = array('value' => $regex ? $conditional->value : $conditional->value[0][$key]);
  }

  $state = array($conditional->state => $addressfield_selectors);
}
