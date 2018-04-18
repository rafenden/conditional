It's a fork of [Conditional Fields](https://www.drupal.org/project/conditional_fields) module that works with any Drupal elements.

Provides two render properties: `#visible_when` and `#required_when`.

Examples:

```php
$form['source'] = [
  '#title' => t('How did you hear about us'),
  '#type' => 'select',
  '#options' => [
    'internet_search' => t('Internet search'),
    'friend' => t('From a friend'),
    'other' => t('Other, please specify below'),
  ],
];

$form['source_other'] = [
  '#title' => t('Other source'),
  '#type' => 'textfield',
  '#visible_when' => [
    'source' => 'other',
  ],
];
```
