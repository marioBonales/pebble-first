module.exports = [
  {
    "type": "heading",
    "defaultValue": "Watchface Settings"
  },
  {
    "type": "text",
    "defaultValue": "Customize your watchface appearance and preferences."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Preferences"
      },
      {
        "type": "select",
        "messageKey": "ShowSeconds",
        "label": "Show seconds",
        "defaultValue": "screenon",
        "options": [
          {
            "label": "Only when screen is on (Saves battery)",
            "value": "screenon"
          },
          {
            "label": "Always",
            "value": "always"
          },
          {
            "label": "Never",
            "value": "never"
          }
        ]
      },
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
