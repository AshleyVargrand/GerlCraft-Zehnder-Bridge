const CARD_VERSION = "1.4.5-final";
const CARD_TYPE = "gerlcraft-zehnder-card-v14";
const DEFAULT_PREFIX = "zehnder_comfoair_q350";

const ENTITY_DEFINITIONS = {
  connection: { domain: "binary_sensor", suffixes: ["verbindung"] },
  fanLevel: { domain: "sensor", suffixes: ["lufterstufe"] },
  operatingMode: { domain: "sensor", suffixes: ["betriebsmodus"] },
  bridgeHealth: { domain: "sensor", suffixes: ["bridge_gesundheit"] },
  filterStatus: { domain: "sensor", suffixes: ["filterstatus"] },
  filterDays: { domain: "sensor", suffixes: ["filterrestlaufzeit"] },
  freeCooling: { domain: "binary_sensor", suffixes: ["freie_kuhlung"] },
  freeCoolingReason: {
    domain: "sensor",
    suffixes: ["freie_kuhlung_begrundung"],
  },
  bypass: { domain: "sensor", suffixes: ["bypass_offnung"] },

  boost: {
    domain: "switch",
    suffixes: ["boost"],
    fallbackEntityIds: ["switch.gerlcraft_zehnder_bridge_boost"],
  },

  boostRemaining: {
    domain: "sensor",
    suffixes: ["boost_restzeit"],
    fallbackEntityIds: [
      "sensor.gerlcraft_zehnder_bridge_boost_restzeit",
      "sensor.gerlcraft_zehnder_bridge_lufterstufe_nachste_anderung",
    ],
  },

  outdoorTemperature: {
    domain: "sensor",
    suffixes: ["aussenlufttemperatur"],
  },
  supplyTemperature: {
    domain: "sensor",
    suffixes: ["zulufttemperatur"],
  },
  extractTemperature: {
    domain: "sensor",
    suffixes: ["ablufttemperatur"],
  },
  exhaustTemperature: {
    domain: "sensor",
    suffixes: ["fortlufttemperatur"],
  },

  supplyFlow: {
    domain: "sensor",
    suffixes: ["zuluftvolumenstrom"],
  },
  extractFlow: {
    domain: "sensor",
    suffixes: ["abluftvolumenstrom"],
  },
  airflowDeviation: {
    domain: "sensor",
    suffixes: ["volumenstrom_abweichung"],
  },

  supplySpeed: {
    domain: "sensor",
    suffixes: ["zuluftventilator_drehzahl"],
  },
  extractSpeed: {
    domain: "sensor",
    suffixes: ["abluftventilator_drehzahl"],
  },

  ventilationPower: {
    domain: "sensor",
    suffixes: ["leistungsaufnahme"],
  },

  heatRecovery: {
    domain: "sensor",
    suffixes: ["warmeruckgewinnungsgrad"],
  },
  coolingRecovery: {
    domain: "sensor",
    suffixes: ["kalteruckgewinnungsgrad"],
  },

  ventilationEnergyYear: {
    domain: "sensor",
    suffixes: ["luftungsenergie_laufendes_jahr"],
  },

  recoveredHeatingEnergy: {
    domain: "sensor",
    suffixes: [
      "eingesparte_heizenergie_gesamt",
      "vermiedene_heizenergie_gesamt",
    ],
  },

  avoidedHeatingPower: {
    domain: "sensor",
    suffixes: [
      "eingesparte_heizleistung",
      "vermiedene_heizleistung",
    ],
  },
};

const INVALID_STATES = new Set([
  "",
  "unknown",
  "unavailable",
  "none",
  "null",
]);

class GerlCraftZehnderCardV14 extends HTMLElement {
  constructor() {
    super();

    this.attachShadow({ mode: "open" });

    this._config = {};
    this._hass = undefined;
    this._prefix = DEFAULT_PREFIX;

    this._boostPending = false;
    this._boostTargetState = undefined;
    this._boostPendingTimeout = undefined;

    this._renderShell();

    this.shadowRoot.addEventListener(
      "click",
      (event) => this._handleClick(event),
    );
  }

  static getStubConfig() {
    return {
      title: "Zehnder ComfoAir Q",
      humidity_warning_threshold: 65,
      full_height: true,
    };
  }

  setConfig(config) {
    this._config = {
      ...config,
      entities: { ...(config.entities || {}) },
    };

    this._setText(
      "title",
      this._config.title || "Zehnder ComfoAir Q",
    );

    this._update();
  }

  set hass(hass) {
    this._hass = hass;
    this._prefix = this._detectPrefix();

    if (
      this._boostPending &&
      this._boostTargetState !== undefined
    ) {
      const state = this._state("boost")?.state;

      const targetReached = this._boostTargetState
        ? state === "on"
        : state === "off";

      if (targetReached) {
        this._clearBoostPending();
      }
    }

    this._update();
  }

  getCardSize() {
    return 12;
  }

  getGridOptions() {
    return {
      columns: "full",
      rows: 12,
      min_columns: 6,
      min_rows: 8,
    };
  }

  _detectPrefix() {
    if (this._config.entity_prefix) {
      return String(this._config.entity_prefix)
        .replace(/^(sensor|binary_sensor)\./, "")
        .replace(/_$/, "");
    }

    if (!this._hass) {
      return DEFAULT_PREFIX;
    }

    const suffix = "_zuluftvolumenstrom";

    const match = Object.keys(this._hass.states).find(
      (entityId) =>
        entityId.startsWith("sensor.") &&
        entityId.endsWith(suffix),
    );

    if (!match) {
      return DEFAULT_PREFIX;
    }

    return match.slice(
      "sensor.".length,
      -suffix.length,
    );
  }

  _entityId(key) {
    if (
      key === "humidity" &&
      this._config.humidity_entity
    ) {
      return String(this._config.humidity_entity);
    }

    if (
      key === "boost" &&
      this._config.boost_entity
    ) {
      return String(this._config.boost_entity);
    }

    if (
      key === "boostRemaining" &&
      this._config.boost_remaining_entity
    ) {
      return String(
        this._config.boost_remaining_entity,
      );
    }

    const configured =
      this._config.entities?.[key];

    if (configured) {
      return configured;
    }

    const definition =
      ENTITY_DEFINITIONS[key];

    if (!definition) {
      return undefined;
    }

    for (const suffix of definition.suffixes) {
      const entityId =
        `${definition.domain}.` +
        `${this._prefix}_${suffix}`;

      if (this._hass?.states?.[entityId]) {
        return entityId;
      }
    }

    for (
      const entityId of
      definition.fallbackEntityIds || []
    ) {
      if (this._hass?.states?.[entityId]) {
        return entityId;
      }
    }

    return (
      `${definition.domain}.` +
      `${this._prefix}_` +
      `${definition.suffixes[0]}`
    );
  }

  _state(key) {
    const entityId = this._entityId(key);

    return entityId
      ? this._hass?.states?.[entityId]
      : undefined;
  }

  _isValid(stateObject) {
    return Boolean(
      stateObject &&
      !INVALID_STATES.has(
        String(stateObject.state).toLowerCase(),
      ),
    );
  }

  _rawNumber(key) {
    const stateObject = this._state(key);

    if (!this._isValid(stateObject)) {
      return undefined;
    }

    const value = Number(stateObject.state);

    return Number.isFinite(value)
      ? value
      : undefined;
  }

  _value(key, digits = 0) {
    const stateObject = this._state(key);

    if (!this._isValid(stateObject)) {
      return "–";
    }

    const numericValue =
      Number(stateObject.state);

    if (!Number.isFinite(numericValue)) {
      return stateObject.state;
    }

    const language =
      this._hass?.locale?.language ||
      "de-DE";

    return numericValue.toLocaleString(
      language,
      {
        minimumFractionDigits: digits,
        maximumFractionDigits: digits,
      },
    );
  }

  _setText(id, value) {
    const element =
      this.shadowRoot.getElementById(id);

    if (element) {
      element.textContent = value;
    }
  }

  _setClickableValue(
    id,
    key,
    digits = 0,
  ) {
    this._setText(
      id,
      this._value(key, digits),
    );

    const target =
      this.shadowRoot
        .getElementById(id)
        ?.closest("[data-key]");

    if (target) {
      target.dataset.key = key;

      target.classList.toggle(
        "unavailable",
        !this._isValid(this._state(key)),
      );
    }
  }

  _formatBoostRemaining(seconds) {
    if (
      !Number.isFinite(seconds) ||
      seconds <= 0
    ) {
      return undefined;
    }

    const totalSeconds =
      Math.max(
        0,
        Math.round(seconds),
      );

    const minutes =
      Math.floor(totalSeconds / 60);

    const remainingSeconds =
      String(totalSeconds % 60)
        .padStart(2, "0");

    return (
      `${minutes}:` +
      `${remainingSeconds} min`
    );
  }

  _getFilterInfo(filterStatus, filterDays) {
    const statusText =
      String(filterStatus || "").trim();

    const statusOk =
      statusText === "OK" ||
      statusText === "–";

    if (
      Number.isFinite(filterDays) &&
      filterDays <= 0
    ) {
      return {
        level: "error",
        label: "Fällig",
        hint: "jetzt wechseln",
      };
    }

    if (!statusOk) {
      return {
        level: "warn",
        label: statusText,
        hint: "Filterstatus meldet Warnung",
      };
    }

    if (
      Number.isFinite(filterDays) &&
      filterDays <= 7
    ) {
      return {
        level: "warn",
        label: "Dringend",
        hint: `nur noch ${Math.max(0, Math.round(filterDays))} Tage`,
      };
    }

    if (
      Number.isFinite(filterDays) &&
      filterDays <= 30
    ) {
      return {
        level: "notice",
        label: "Bald fällig",
        hint: `noch ${Math.round(filterDays)} Tage`,
      };
    }

    if (Number.isFinite(filterDays)) {
      return {
        level: "ok",
        label: "OK",
        hint: `noch ${Math.round(filterDays)} Tage`,
      };
    }

    return {
      level: statusOk ? "ok" : "warn",
      label: statusOk ? "OK" : statusText,
      hint: statusOk ? "Restlaufzeit unbekannt" : "Filterstatus prüfen",
    };
  }

  _update() {
    if (!this._hass) {
      return;
    }

    const connected =
      this._state("connection")?.state === "on";

    const bridgeHealth =
      this._value("bridgeHealth");

    const filterStatus =
      this._value("filterStatus");

    const filterDays =
      this._rawNumber("filterDays");

    const freeCooling =
      this._state("freeCooling")?.state === "on";

    const bypass =
      this._rawNumber("bypass") ?? 0;

    const heatRecovery =
      this._rawNumber("heatRecovery");

    const coolingRecovery =
      this._rawNumber("coolingRecovery");

    const currentRecoveredPower =
      this._rawNumber("avoidedHeatingPower");

    const ventilationPower =
      this._rawNumber("ventilationPower");

    const outdoorTemp =
      this._rawNumber("outdoorTemperature");

    const supplyTemp =
      this._rawNumber("supplyTemperature");

    const supplyFlow =
      this._rawNumber("supplyFlow") ?? 0;

    const extractFlow =
      this._rawNumber("extractFlow") ?? 0;

    const supplySpeed =
      this._rawNumber("supplySpeed") ?? 0;

    const extractSpeed =
      this._rawNumber("extractSpeed") ?? 0;

    const averageFlow =
      (supplyFlow + extractFlow) / 2;

    const averageSpeed =
      (supplySpeed + extractSpeed) / 2;

    const boostState =
      this._state("boost");

    const boostAvailable =
      this._isValid(boostState);

    const boostActive =
      boostAvailable &&
      boostState.state === "on";

    const boostRemaining =
      this._rawNumber("boostRemaining");

    const humidityId =
      this._entityId("humidity");

    const humidityState =
      humidityId
        ? this._hass.states[humidityId]
        : undefined;

    const humidityConfigured =
      Boolean(humidityId);

    const humidityAvailable =
      this._isValid(humidityState);

    const humidityValue =
      humidityAvailable
        ? Number(humidityState.state)
        : undefined;

    const configuredThreshold =
      Number(
        this._config
          .humidity_warning_threshold,
      );

    const humidityThreshold =
      Number.isFinite(configuredThreshold)
        ? Math.max(
            1,
            Math.min(
              100,
              configuredThreshold,
            ),
          )
        : 65;

    const humidityHigh =
      Number.isFinite(humidityValue) &&
      humidityValue >= humidityThreshold;

    const card =
      this.shadowRoot.querySelector(
        "ha-card",
      );

    card.classList.toggle(
      "connected",
      connected,
    );

    card.classList.toggle(
      "disconnected",
      !connected,
    );

    card.classList.toggle(
      "boost-active",
      boostActive,
    );

    card.classList.toggle(
      "free-cooling",
      freeCooling,
    );

    card.classList.toggle(
      "bypass-active",
      bypass > 10,
    );

    card.classList.toggle(
      "full-height",
      this._config.full_height !== false,
    );

    card.style.setProperty(
      "--flow-duration",
      `${Math.max(
        0.9,
        Math.min(
          4.2,
          4.35 - averageFlow / 105,
        ),
      ).toFixed(2)}s`,
    );

    card.style.setProperty(
      "--fan-duration",
      `${Math.max(
        0.7,
        Math.min(
          4.0,
          4.5 - averageSpeed / 650,
        ),
      ).toFixed(2)}s`,
    );

    const heroFanDuration = boostActive
      ? Math.max(
          4.4,
          Math.min(
            5.4,
            5.6 - averageSpeed / 4000,
          ),
        )
      : Math.max(
          7.0,
          Math.min(
            9.2,
            9.4 - averageSpeed / 1800,
          ),
        );

    card.style.setProperty(
      "--hero-fan-duration",
      `${heroFanDuration.toFixed(2)}s`,
    );

    card.style.setProperty(
      "--bypass-angle",
      `${Math.min(
        82,
        Math.max(0, bypass * 0.82),
      )}deg`,
    );

    card.style.setProperty(
      "--bypass-opacity",
      String(
        Math.max(
          0.12,
          Math.min(1, bypass / 100),
        ),
      ),
    );

    this._setText(
      "connection",
      connected
        ? "Verbunden"
        : "Getrennt",
    );

    this._setText(
      "bridge-health",
      bridgeHealth,
    );

    this._setText(
      "filter-status",
      this._getFilterInfo(
        filterStatus,
        filterDays,
      ).label,
    );

    this._setText(
      "fan-level",
      this._value("fanLevel"),
    );

    this._setText(
      "fan-level-side",
      this._value("fanLevel"),
    );

    this._setText(
      "operating-mode",
      this._value("operatingMode"),
    );

    this._setText(
      "operating-mode-side",
      this._value("operatingMode"),
    );

    this._setText(
      "free-cooling",
      freeCooling
        ? "Freie Kühlung"
        : "Standardbetrieb",
    );

    this._setText(
      "free-cooling-reason",
      this._value(
        "freeCoolingReason",
      ),
    );

    this._setText(
      "outdoor-temperature",
      this._value(
        "outdoorTemperature",
        1,
      ),
    );

    this._setText(
      "supply-temperature",
      this._value(
        "supplyTemperature",
        1,
      ),
    );

    this._setText(
      "extract-temperature",
      this._value(
        "extractTemperature",
        1,
      ),
    );

    this._setText(
      "exhaust-temperature",
      this._value(
        "exhaustTemperature",
        1,
      ),
    );

    this._setText(
      "humidity",
      !humidityConfigured
        ? "Nicht eingerichtet"
        : !humidityAvailable ||
            !Number.isFinite(
              humidityValue,
            )
          ? "Offline"
          : `${this._value(
              "humidity",
            )} %`,
    );

    this._setClickableValue(
      "supply-flow",
      "supplyFlow",
    );

    this._setClickableValue(
      "extract-flow",
      "extractFlow",
    );

    this._setClickableValue(
      "airflow-deviation",
      "airflowDeviation",
      1,
    );

    this._setClickableValue(
      "ventilation-power",
      "ventilationPower",
    );

    this._setClickableValue(
      "supply-speed",
      "supplySpeed",
    );

    this._setClickableValue(
      "extract-speed",
      "extractSpeed",
    );

    this._setClickableValue(
      "filter-days",
      "filterDays",
    );

    this._setClickableValue(
      "ventilation-energy",
      "ventilationEnergyYear",
    );

    this._setClickableValue(
      "recovered-heating-energy",
      "recoveredHeatingEnergy",
    );

    this._setClickableValue(
      "avoided-heating-power",
      "avoidedHeatingPower",
    );

    const energyRatio =
      Number.isFinite(currentRecoveredPower) &&
      Number.isFinite(ventilationPower) &&
      ventilationPower > 0
        ? currentRecoveredPower / ventilationPower
        : undefined;

    const temperatureLift =
      Number.isFinite(outdoorTemp) &&
      Number.isFinite(supplyTemp)
        ? supplyTemp - outdoorTemp
        : undefined;

    const temperatureLiftText =
      Number.isFinite(temperatureLift)
        ? `${temperatureLift >= 0 ? "+" : ""}${temperatureLift.toLocaleString(
            this._hass?.locale?.language || "de-DE",
            {
              minimumFractionDigits: 1,
              maximumFractionDigits: 1,
            },
          )} K`
        : "–";

    this._setText(
      "temperature-lift",
      temperatureLiftText,
    );

    this._setText(
      "temperature-lift-side",
      temperatureLiftText,
    );

    this._setText(
      "bypass-value",
      `${this._value("bypass")} %`,
    );

    this._updateRecovery(
      "heat-recovery",
      "heatRecovery",
      bypass,
    );

    this._updateRecovery(
      "cooling-recovery",
      "coolingRecovery",
      bypass,
    );

    this._updateCenterState(
      bypass,
      heatRecovery,
      coolingRecovery,
      currentRecoveredPower,
      freeCooling,
    );

    this._updateBoostControl(
      connected,
      boostAvailable,
      boostActive,
      boostRemaining,
    );

    const connectionBadge =
      this.shadowRoot.getElementById(
        "connection-badge",
      );

    connectionBadge?.classList.toggle(
      "ok",
      connected,
    );

    connectionBadge?.classList.toggle(
      "error",
      !connected,
    );

    const bridgeBadge =
      this.shadowRoot.getElementById(
        "bridge-badge",
      );

    bridgeBadge?.classList.toggle(
      "ok",
      bridgeHealth === "OK",
    );

    bridgeBadge?.classList.toggle(
      "warn",
      bridgeHealth !== "OK",
    );

    const filterInfo =
      this._getFilterInfo(
        filterStatus,
        filterDays,
      );

    this._setText(
      "filter-hint",
      filterInfo.hint,
    );

    const filterBadge =
      this.shadowRoot.getElementById(
        "filter-badge",
      );

    filterBadge?.classList.toggle(
      "ok",
      filterInfo.level === "ok",
    );

    filterBadge?.classList.toggle(
      "notice",
      filterInfo.level === "notice",
    );

    filterBadge?.classList.toggle(
      "warn",
      filterInfo.level === "warn",
    );

    filterBadge?.classList.toggle(
      "error",
      filterInfo.level === "error",
    );

    const filterItem =
      this.shadowRoot.getElementById(
        "filter-item",
      );

    filterItem?.classList.toggle(
      "notice",
      filterInfo.level === "notice",
    );

    filterItem?.classList.toggle(
      "warn",
      filterInfo.level === "warn",
    );

    filterItem?.classList.toggle(
      "error",
      filterInfo.level === "error",
    );

    const humidityBadge =
      this.shadowRoot.getElementById(
        "humidity-badge",
      );

    humidityBadge?.classList.toggle(
      "muted",
      !humidityConfigured,
    );

    humidityBadge?.classList.toggle(
      "error",
      humidityConfigured &&
      (
        !humidityAvailable ||
        !Number.isFinite(
          humidityValue,
        )
      ),
    );

    humidityBadge?.classList.toggle(
      "warn",
      humidityConfigured &&
      humidityAvailable &&
      humidityHigh,
    );

    humidityBadge?.classList.toggle(
      "ok",
      humidityConfigured &&
      humidityAvailable &&
      !humidityHigh,
    );

    if (humidityBadge) {
      humidityBadge.dataset.key =
        humidityConfigured
          ? "humidity"
          : "";
    }
  }

  _updateCenterState(
    bypass,
    heatRecovery,
    coolingRecovery,
    currentRecoveredPower,
    freeCooling,
  ) {
    let mode = "Standby";
    let value = "–";
    let unit = "";
    let hint = "Aktueller Anlagenzustand";

    if (
      Number.isFinite(heatRecovery)
    ) {
      mode = "Wärmerückgewinnung";
      value =
        heatRecovery.toLocaleString(
          this._hass?.locale?.language ||
            "de-DE",
          {
            minimumFractionDigits: 1,
            maximumFractionDigits: 1,
          },
        );
      unit = "%";

      hint =
        Number.isFinite(
          currentRecoveredPower,
        )
          ? `${Math.round(
              currentRecoveredPower,
            )} W aktuell eingespart`
          : "Wärmetauscher aktiv";
    } else if (
      Number.isFinite(
        coolingRecovery,
      )
    ) {
      mode = "Kälterückgewinnung";
      value =
        coolingRecovery.toLocaleString(
          this._hass?.locale?.language ||
            "de-DE",
          {
            minimumFractionDigits: 1,
            maximumFractionDigits: 1,
          },
        );
      unit = "%";
      hint = "Kälterückgewinnung aktiv";
    } else if (bypass > 10) {
      mode = "Bypass";
      value =
        Math.round(bypass).toString();
      unit = "%";

      hint = freeCooling
        ? "Freie Kühlung aktiv"
        : "Wärmetauscher umgangen";
    }

    this._setText(
      "hero-mode",
      mode,
    );

    this._setText(
      "hero-value",
      value,
    );

    this._setText(
      "hero-unit",
      unit,
    );

    this._setText(
      "hero-hint",
      hint,
    );

    // Der Lüfter bleibt bewusst in jedem Betriebszustand das zentrale Symbol,
    // dreht aber deutlich ruhiger als die realen RPM.
    const heroIcon =
      this.shadowRoot.getElementById(
        "hero-icon",
      );

    if (heroIcon) {
      heroIcon.setAttribute(
        "icon",
        "mdi:fan",
      );
    }
  }

  _updateRecovery(
    id,
    key,
    bypass,
  ) {
    const stateObject =
      this._state(key);

    const valid =
      this._isValid(stateObject);

    const element =
      this.shadowRoot.getElementById(id);

    const container =
      element?.closest("[data-key]");

    if (!element || !container) {
      return;
    }

    element.textContent =
      valid
        ? `${this._value(key, 1)} %`
        : "–";

    container.classList.toggle(
      "unavailable",
      !valid,
    );

    const hint =
      container.querySelector(
        ".detail-hint",
      );

    if (hint) {
      hint.textContent =
        valid
          ? "Aktueller Wirkungsgrad"
          : bypass > 10
            ? "Bypass aktiv"
            : "Derzeit nicht aktiv";
    }
  }

  _updateBoostControl(
    connected,
    available,
    active,
    remainingSeconds,
  ) {
    const button =
      this.shadowRoot.getElementById(
        "boost-control",
      );

    const icon =
      this.shadowRoot.getElementById(
        "boost-control-icon",
      );

    if (!button || !icon) {
      return;
    }

    const ready =
      connected &&
      available &&
      !this._boostPending;

    const formattedRemaining =
      active
        ? this._formatBoostRemaining(
            remainingSeconds,
          )
        : undefined;

    const label =
      this._boostPending
        ? "Wird geschaltet …"
        : !connected
          ? "Nicht verbunden"
          : !available
            ? "Nicht verfügbar"
            : active
              ? "Boost beenden"
              : "Boost starten";

    const sub =
      this._boostPending
        ? "Befehl wird gesendet"
        : !connected
          ? "Bridge nicht verbunden"
          : !available
            ? "Boost nicht verfügbar"
            : active
              ? formattedRemaining
                ? `Restzeit ${formattedRemaining}`
                : "Party-Timer aktiv"
              : "60-Minuten-Boost";

    button.disabled = !ready;

    button.classList.toggle(
      "active",
      active,
    );

    button.setAttribute(
      "aria-pressed",
      String(active),
    );

    button.setAttribute(
      "aria-label",
      label,
    );

    button.title = label;

    icon.setAttribute(
      "icon",
      active
        ? "mdi:stop-circle-outline"
        : "mdi:fan-clock",
    );

    this._setText(
      "boost-control-label",
      label,
    );

    this._setText(
      "boost-control-sub",
      sub,
    );
  }

  _clearBoostPending() {
    if (
      this._boostPendingTimeout !==
      undefined
    ) {
      clearTimeout(
        this._boostPendingTimeout,
      );
    }

    this._boostPending = false;
    this._boostTargetState = undefined;
    this._boostPendingTimeout = undefined;
  }

  async _toggleBoost() {
    if (
      !this._hass ||
      this._boostPending
    ) {
      return;
    }

    const entityId =
      this._entityId("boost");

    const stateObject =
      entityId
        ? this._hass.states[entityId]
        : undefined;

    if (
      !entityId ||
      !this._isValid(stateObject)
    ) {
      return;
    }

    const active =
      stateObject.state === "on";

    this._boostPending = true;
    this._boostTargetState = !active;

    this._update();

    try {
      await this._hass.callService(
        "switch",
        active
          ? "turn_off"
          : "turn_on",
        {
          entity_id: entityId,
        },
      );

      if (this._boostPending) {
        this._boostPendingTimeout =
          setTimeout(() => {
            this._clearBoostPending();
            this._update();
          }, 12000);
      }
    } catch (error) {
      console.error(
        "GerlCraft Zehnder Card v1.3 Test: Boost Fehler",
        error,
      );

      this._clearBoostPending();
      this._update();
    }
  }

  _handleClick(event) {
    const actionTarget =
      event.target.closest(
        "[data-action]",
      );

    if (
      actionTarget?.dataset.action ===
      "toggle-boost"
    ) {
      event.preventDefault();
      event.stopPropagation();
      this._toggleBoost();
      return;
    }

    const target =
      event.target.closest(
        "[data-key]",
      );

    if (
      !target ||
      !this._hass
    ) {
      return;
    }

    const key =
      target.dataset.key;

    if (!key) {
      return;
    }

    const entityId =
      this._entityId(key);

    if (
      !entityId ||
      !this._hass.states[entityId]
    ) {
      return;
    }

    this.dispatchEvent(
      new CustomEvent(
        "hass-more-info",
        {
          bubbles: true,
          composed: true,
          detail: {
            entityId,
          },
        },
      ),
    );
  }

  _renderShell() {
    this.shadowRoot.innerHTML = `
      <style>
        :host {
          display: block;
          --gc-accent: var(--primary-color, #4aa3ff);
          --gc-cold: #22b7f2;
          --gc-cold-soft: #71d7ff;
          --gc-warm: #ffb21c;
          --gc-hot: #ff704d;
          --gc-good: var(--success-color, #5ac878);
          --flow-duration: 2.1s;
          --fan-duration: 2.2s;
          --hero-fan-duration: 8.5s;
        }

        * {
          box-sizing: border-box;
        }

        button {
          font: inherit;
          color: inherit;
        }

        ha-card {
          min-width: 0;
          overflow: hidden;
          color: var(--primary-text-color);
          border: 1px solid color-mix(in srgb, var(--divider-color) 80%, var(--gc-accent) 20%);
          border-radius: 20px;
          background:
            radial-gradient(circle at 44% 34%, color-mix(in srgb, var(--gc-accent) 5%, transparent), transparent 34%),
            radial-gradient(circle at 4% 10%, color-mix(in srgb, var(--gc-cold) 3%, transparent), transparent 26%),
            var(--ha-card-background, var(--card-background-color));
          box-shadow:
            0 18px 50px rgba(0,0,0,.15),
            inset 0 1px 0 color-mix(in srgb, var(--primary-text-color) 5%, transparent);
        }

        ha-card.full-height {
          min-height: calc(100vh - 18px);
        }

        .shell {
          position: relative;
          display: flex;
          flex-direction: column;
          width: 100%;
          min-height: inherit;
          padding: 20px;
        }

        ha-card.full-height .shell {
          min-height: calc(100vh - 18px);
        }

        /* ---------- HEADER ---------- */

        .header {
          display: grid;
          grid-template-columns: minmax(0, 1fr) auto;
          align-items: center;
          gap: 18px;
          padding-bottom: 16px;
          border-bottom: 1px solid var(--divider-color);
        }

        .identity {
          display: flex;
          align-items: center;
          gap: 14px;
          min-width: 0;
        }

        .identity-icon {
          display: grid;
          place-items: center;
          width: 54px;
          height: 54px;
          flex: 0 0 54px;
          border-radius: 16px;
          color: var(--gc-accent);
          border: 1px solid color-mix(in srgb, var(--gc-accent) 25%, transparent);
          background: color-mix(in srgb, var(--gc-accent) 10%, transparent);
        }

        .identity-icon ha-icon {
          width: 28px;
          height: 28px;
          --mdc-icon-size: 28px;
        }

        .connected .identity-icon ha-icon {
          animation: spin var(--fan-duration) linear infinite;
        }

        h2 {
          margin: 0;
          font-size: 27px;
          line-height: 1.1;
          font-weight: 780;
        }

        .subtitle {
          display: flex;
          flex-wrap: wrap;
          align-items: center;
          gap: 7px;
          margin-top: 6px;
          color: var(--secondary-text-color);
          font-size: 13px;
        }

        .mini-pill {
          display: inline-flex;
          align-items: center;
          gap: 5px;
          min-height: 27px;
          padding: 4px 9px;
          border-radius: 999px;
          border: 1px solid var(--divider-color);
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
          font-size: 11px;
        }

        .mini-pill strong {
          color: var(--primary-text-color);
        }

        .statusbar {
          display: flex;
          flex-wrap: wrap;
          justify-content: flex-end;
          gap: 10px;
        }

        .status-chip {
          display: flex;
          align-items: center;
          gap: 10px;
          min-height: 52px;
          padding: 9px 15px;
          border: 1px solid var(--divider-color);
          border-radius: 13px;
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
          cursor: pointer;
        }

        .status-chip ha-icon {
          width: 22px;
          height: 22px;
          --mdc-icon-size: 22px;
          color: var(--secondary-text-color);
        }

        .chip-copy {
          display: grid;
          line-height: 1.05;
        }

        .chip-label {
          color: var(--secondary-text-color);
          font-size: 11px;
          text-transform: uppercase;
          letter-spacing: .06em;
        }

        .chip-value {
          margin-top: 5px;
          font-size: 16px;
          font-weight: 740;
        }

        .status-chip.ok { color: var(--success-color); }
        .status-chip.notice { color: var(--warning-color); }
        .status-chip.warn { color: var(--warning-color); }
        .status-chip.error { color: var(--error-color); }
        .status-chip.muted { opacity: .65; }

        /* ---------- MAIN ---------- */

        .main {
          display: grid;
          grid-template-columns: minmax(0, 1.42fr) minmax(430px, .82fr);
          gap: 18px;
          flex: 1 1 auto;
          min-height: 0;
          padding: 18px 0 14px;
        }

        .schematic,
        .performance {
          min-width: 0;
          height: 100%;
          border: 1px solid var(--divider-color);
          border-radius: 17px;
          background: color-mix(in srgb, var(--primary-text-color) 1.8%, transparent);
        }

        .schematic {
          display: flex;
          flex-direction: column;
          min-height: 560px;
          padding: 17px;
          overflow: hidden;
        }

        .section-head {
          display: flex;
          align-items: flex-start;
          justify-content: space-between;
          gap: 12px;
        }

        .section-title {
          color: var(--secondary-text-color);
          font-size: 12px;
          font-weight: 760;
          letter-spacing: .075em;
          text-transform: uppercase;
        }

        .section-sub {
          margin-top: 4px;
          color: var(--secondary-text-color);
          font-size: 10px;
        }

        .signature-badge {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          padding: 5px 9px;
          border: 1px solid color-mix(in srgb, var(--gc-accent) 22%, var(--divider-color));
          border-radius: 999px;
          color: var(--gc-accent);
          background: color-mix(in srgb, var(--gc-accent) 6%, transparent);
          font-size: 9px;
          font-weight: 700;
          letter-spacing: .05em;
          text-transform: uppercase;
        }

        /* ---------- AIRFLOW SCHEMATIC ---------- */

        .airspace {
          position: relative;
          flex: 1 1 auto;
          min-height: 490px;
          margin-top: 8px;
        }

        .flow-svg {
          position: absolute;
          inset: 0;
          width: 100%;
          height: 100%;
          pointer-events: none;
          overflow: visible;
        }

        .flow-base {
          fill: none;
          stroke-width: 18;
          stroke-linecap: round;
          opacity: .09;
        }

        .flow-edge {
          fill: none;
          stroke-width: 2;
          stroke-linecap: round;
          opacity: .32;
        }

        .flow-motion {
          fill: none;
          stroke-width: 7;
          stroke-linecap: round;
          stroke-dasharray: 1 24;
          opacity: .9;
        }

        .flow-cold { stroke: var(--gc-cold); }
        .flow-warm { stroke: var(--gc-warm); }
        .flow-hot { stroke: var(--gc-hot); }

        .connected .flow-motion.forward {
          animation: dash-forward var(--flow-duration) linear infinite;
        }

        .connected .flow-motion.reverse {
          animation: dash-reverse var(--flow-duration) linear infinite;
        }

        .node {
          position: absolute;
          z-index: 4;
          display: grid;
          justify-items: center;
          align-content: center;
          gap: 5px;
          width: 138px;
          height: 118px;
          padding: 12px 10px;
          border: 1px solid color-mix(in srgb, currentColor 22%, var(--divider-color));
          border-radius: 22px;
          background:
            radial-gradient(circle at 50% 0%, color-mix(in srgb, currentColor 10%, transparent), transparent 65%),
            color-mix(in srgb, var(--ha-card-background, var(--card-background-color)) 88%, transparent);
          text-align: center;
          cursor: pointer;
          box-shadow: inset 0 1px 0 color-mix(in srgb, var(--primary-text-color) 4%, transparent);
        }

        .node ha-icon {
          width: 34px;
          height: 34px;
          --mdc-icon-size: 34px;
        }

        .node-label {
          color: var(--secondary-text-color);
          font-size: 13px;
          font-weight: 600;
        }

        .node-value {
          font-size: 26px;
          line-height: 1;
          font-weight: 780;
          white-space: nowrap;
        }

        .node-hint {
          color: var(--secondary-text-color);
          font-size: 10px;
        }

        .outdoor {
          left: 8px;
          top: calc(26% - 59px);
          color: var(--gc-cold);
        }

        .supply {
          right: 8px;
          top: calc(26% - 59px);
          color: var(--gc-cold);
        }

        .exhaust {
          left: 8px;
          top: calc(74% - 59px);
          color: var(--gc-hot);
        }

        .extract {
          right: 8px;
          top: calc(74% - 59px);
          color: var(--gc-warm);
        }

        /* ---------- CENTRAL EXCHANGER ---------- */

        .core {
          position: absolute;
          z-index: 5;
          left: 50%;
          top: 50%;
          width: 330px;
          min-height: 390px;
          transform: translate(-50%, -50%);
          display: grid;
          justify-items: center;
          align-content: center;
          gap: 11px;
          padding: 22px 20px;
          border: 1px solid color-mix(in srgb, var(--divider-color) 68%, var(--gc-accent) 32%);
          border-radius: 32px;
          background:
            radial-gradient(circle at 50% 28%, color-mix(in srgb, var(--gc-accent) 11%, transparent), transparent 40%),
            linear-gradient(180deg, color-mix(in srgb, var(--primary-text-color) 3%, transparent), transparent),
            var(--ha-card-background, var(--card-background-color));
          box-shadow:
            0 22px 45px rgba(0,0,0,.16),
            inset 0 1px 0 color-mix(in srgb, var(--primary-text-color) 6%, transparent);
        }

        .core::before {
          content: "";
          position: absolute;
          inset: 12px;
          border: 1px solid color-mix(in srgb, var(--gc-accent) 9%, transparent);
          border-radius: 24px;
          pointer-events: none;
        }

        .hx-visual {
          position: relative;
          display: grid;
          place-items: center;
          width: 178px;
          height: 178px;
          border-radius: 38px;
          border: 1px solid color-mix(in srgb, var(--gc-accent) 45%, transparent);
          background:
            linear-gradient(135deg, color-mix(in srgb, var(--gc-cold) 7%, transparent), transparent 44%, color-mix(in srgb, var(--gc-warm) 7%, transparent)),
            color-mix(in srgb, var(--gc-accent) 3%, transparent);
          overflow: hidden;
        }

        .hx-visual::before,
        .hx-visual::after {
          content: "";
          position: absolute;
          width: 225px;
          height: 2px;
          background: color-mix(in srgb, var(--gc-accent) 31%, transparent);
          transform: rotate(45deg);
        }

        .hx-visual::after {
          transform: rotate(-45deg);
        }

        .thermal-cold,
        .thermal-warm {
          position: absolute;
          width: 145%;
          height: 24px;
          border-radius: 999px;
          opacity: .18;
        }

        .thermal-cold {
          background: var(--gc-cold);
          transform: rotate(28deg);
        }

        .thermal-warm {
          background: var(--gc-warm);
          transform: rotate(-28deg);
        }

        .hx-visual ha-icon {
          position: relative;
          z-index: 3;
          width: 78px;
          height: 78px;
          --mdc-icon-size: 78px;
          color: var(--gc-accent);
          filter: drop-shadow(0 0 9px color-mix(in srgb, var(--gc-accent) 30%, transparent));
        }


        .connected .hx-visual ha-icon {
          animation: spin var(--hero-fan-duration) linear infinite;
        }

        .boost-active .core {
          border-color: color-mix(in srgb, var(--warning-color) 55%, var(--divider-color));
          box-shadow:
            0 22px 45px rgba(0,0,0,.16),
            0 0 28px color-mix(in srgb, var(--warning-color) 12%, transparent);
        }

        .boost-active .hx-visual {
          border-color: var(--warning-color);
        }

        .core-mode {
          margin-top: 2px;
          color: var(--secondary-text-color);
          font-size: 11px;
          font-weight: 740;
          letter-spacing: .08em;
          text-transform: uppercase;
        }

        .core-number {
          display: flex;
          align-items: baseline;
          gap: 5px;
        }

        .core-value {
          font-size: 55px;
          line-height: .95;
          font-weight: 790;
        }

        .core-unit {
          color: var(--secondary-text-color);
          font-size: 20px;
          font-weight: 650;
        }

        .core-hint {
          min-height: 18px;
          color: var(--secondary-text-color);
          font-size: 12px;
          text-align: center;
        }

        .core-meta {
          display: flex;
          flex-wrap: wrap;
          justify-content: center;
          gap: 7px;
        }

        .core-meta-pill {
          display: inline-flex;
          align-items: center;
          gap: 5px;
          min-height: 29px;
          padding: 4px 9px;
          border-radius: 999px;
          border: 1px solid var(--divider-color);
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
          color: var(--secondary-text-color);
          font-size: 10px;
        }

        .core-meta-pill strong {
          color: var(--primary-text-color);
          font-weight: 720;
        }

        .boost-control {
          display: inline-flex;
          align-items: center;
          justify-content: center;
          gap: 7px;
          min-height: 43px;
          padding: 7px 14px;
          border: 1px solid color-mix(in srgb, var(--gc-accent) 55%, var(--divider-color));
          border-radius: 999px;
          background: color-mix(in srgb, var(--gc-accent) 8%, transparent);
          color: var(--gc-accent);
          font-size: 13px;
          font-weight: 740;
          cursor: pointer;
          transition: .2s ease;
        }

        .boost-control:hover:not(:disabled) {
          transform: translateY(-1px);
          background: color-mix(in srgb, var(--gc-accent) 14%, transparent);
        }

        .boost-control.active {
          color: var(--warning-color);
          border-color: color-mix(in srgb, var(--warning-color) 60%, var(--divider-color));
          background: color-mix(in srgb, var(--warning-color) 10%, transparent);
        }

        .boost-control:disabled {
          opacity: .55;
          cursor: default;
        }

        .boost-control ha-icon {
          width: 19px;
          height: 19px;
          --mdc-icon-size: 19px;
        }

        .boost-sub {
          color: var(--secondary-text-color);
          font-size: 9px;
        }

        /* ---------- PERFORMANCE PANEL ---------- */

        .performance {
          display: flex;
          flex-direction: column;
          padding: 16px;
        }

        .live {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          color: var(--success-color);
          font-size: 10px;
          font-weight: 700;
        }

        .live::before {
          content: "";
          width: 7px;
          height: 7px;
          border-radius: 50%;
          background: currentColor;
          box-shadow: 0 0 9px color-mix(in srgb, currentColor 50%, transparent);
        }


        .detail-hint {
          margin-top: 5px;
          color: var(--secondary-text-color);
          font-size: 9px;
        }

        .unavailable .metric-value {
          color: var(--disabled-text-color);
        }


        /* ---------- CLASSIC LIVE PANEL ---------- */

        .performance {
          display: flex;
          flex-direction: column;
          padding: 21px;
        }

        .live {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          color: var(--success-color);
          font-size: 10px;
          font-weight: 700;
        }

        .live::before {
          content: "";
          width: 7px;
          height: 7px;
          border-radius: 50%;
          background: currentColor;
          box-shadow: 0 0 9px color-mix(in srgb, currentColor 50%, transparent);
        }

        .classic-mode-grid {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: 1px;
          margin-top: 17px;
          overflow: hidden;
          border: 1px solid var(--divider-color);
          border-radius: 13px;
          background: var(--divider-color);
        }

        .classic-mode {
          min-width: 0;
          padding: 22px 20px;
          background: var(--ha-card-background, var(--card-background-color));
        }

        .classic-mode-label {
          color: var(--secondary-text-color);
          font-size: 14px;
        }

        .classic-mode-value {
          display: block;
          margin-top: 8px;
          overflow: hidden;
          font-size: 31px;
          font-weight: 770;
          text-overflow: ellipsis;
          white-space: nowrap;
        }

        .classic-metric-grid {
          display: grid;
          grid-template-columns: 1fr 1fr;
          margin-top: 15px;
          overflow: hidden;
          border: 1px solid var(--divider-color);
          border-radius: 13px;
        }

        .classic-metric {
          display: grid;
          grid-template-columns: 38px minmax(0, 1fr);
          grid-template-areas:
            "icon label"
            "icon value";
          align-items: center;
          min-height: 126px;
          padding: 18px 17px;
          border-right: 1px solid var(--divider-color);
          border-bottom: 1px solid var(--divider-color);
          background: transparent;
          cursor: pointer;
          transition: background .18s ease;
        }

        .classic-metric:nth-child(even) {
          border-right: 0;
        }

        .classic-metric:nth-last-child(-n+2) {
          border-bottom: 0;
        }

        .classic-metric:hover {
          background: color-mix(in srgb, var(--primary-text-color) 4%, transparent);
        }

        .classic-metric-icon {
          grid-area: icon;
          display: grid;
          place-items: center;
          width: 36px;
          height: 36px;
          border-radius: 10px;
          color: var(--secondary-text-color);
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
        }

        .classic-metric-icon ha-icon {
          width: 23px;
          height: 23px;
          --mdc-icon-size: 23px;
        }

        .classic-metric-label {
          grid-area: label;
          color: var(--secondary-text-color);
          font-size: 14px;
        }

        .classic-metric-value {
          grid-area: value;
          margin-top: 7px;
          overflow: hidden;
          font-size: 30px;
          font-weight: 760;
          text-overflow: ellipsis;
          white-space: nowrap;
        }

        .classic-metric-value small {
          margin-left: 5px;
          color: var(--secondary-text-color);
          font-size: 13px;
          font-weight: 500;
        }

        .classic-footer-info {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: 12px;
          margin-top: 15px;
        }

        .classic-footer-item {
          min-width: 0;
          padding: 17px 18px;
          border: 1px solid var(--divider-color);
          border-radius: 12px;
          background: color-mix(in srgb, var(--primary-text-color) 2%, transparent);
          cursor: pointer;
        }

        .classic-footer-item > span {
          color: var(--secondary-text-color);
          font-size: 13px;
        }

        .classic-footer-item strong {
          display: block;
          margin-top: 7px;
          font-size: 25px;
          font-weight: 740;
          white-space: nowrap;
        }

        .classic-footer-item strong small {
          margin-left: 5px;
          color: var(--secondary-text-color);
          font-size: 12px;
          font-weight: 500;
        }

        /* ---------- BOTTOM STRIP ---------- */

        .bottom {
          display: grid;
          grid-template-columns: 1.15fr 1fr 1fr 1.3fr;
          gap: 1px;
          overflow: hidden;
          border: 1px solid var(--divider-color);
          border-radius: 15px;
          background: var(--divider-color);
        }

        .bottom-item {
          min-width: 0;
          padding: 14px 16px;
          background: var(--ha-card-background, var(--card-background-color));
          cursor: pointer;
        }

        .bottom-label {
          display: flex;
          align-items: center;
          gap: 7px;
          color: var(--secondary-text-color);
          font-size: 10px;
        }

        .bottom-label ha-icon {
          width: 19px;
          height: 19px;
          --mdc-icon-size: 19px;
        }

        .bottom-value {
          margin-top: 7px;
          font-size: 21px;
          font-weight: 760;
          white-space: nowrap;
        }

        .bottom-value small {
          margin-left: 3px;
          color: var(--secondary-text-color);
          font-size: 9px;
          font-weight: 500;
        }

        .bottom-hint {
          margin-top: 4px;
          color: var(--secondary-text-color);
          font-size: 9px;
        }

        .bottom-item.notice {
          color: var(--warning-color);
        }

        .bottom-item.warn {
          color: var(--warning-color);
        }

        .bottom-item.error {
          color: var(--error-color);
        }

        .bottom-item.notice .bottom-label,
        .bottom-item.warn .bottom-label,
        .bottom-item.error .bottom-label,
        .bottom-item.notice .bottom-hint,
        .bottom-item.warn .bottom-hint,
        .bottom-item.error .bottom-hint {
          color: currentColor;
        }

        /* ---------- RESPONSIVE ---------- */

        @media (max-width: 1150px) {
          .main {
            grid-template-columns: 1fr;
          }

          .performance {
            height: auto;
          }
        }

        @media (max-width: 900px) {
          .bottom {
            grid-template-columns: 1fr 1fr;
          }

          .core {
            width: 285px;
          }
        }

        @media (max-width: 720px) {
          .shell {
            padding: 13px;
          }

          .header {
            grid-template-columns: 1fr;
          }

          .statusbar {
            justify-content: flex-start;
          }

          .schematic {
            min-height: 500px;
            padding: 12px;
          }

          .airspace {
            min-height: 440px;
          }

          .node {
            width: 92px;
            height: 92px;
            min-height: 92px;
            padding: 8px 5px;
            border-radius: 18px;
          }

          .node ha-icon {
            width: 24px;
            height: 24px;
            --mdc-icon-size: 24px;
          }

          .node-label {
            font-size: 9px;
          }

          .node-value {
            font-size: 16px;
          }

          .node-hint {
            display: none;
          }

          .outdoor,
          .exhaust {
            left: 1px;
          }

          .supply,
          .extract {
            right: 1px;
          }

          .outdoor,
          .supply {
            top: calc(26% - 46px);
          }

          .exhaust,
          .extract {
            top: calc(74% - 46px);
          }

          .core {
            width: 210px;
            min-height: 315px;
            padding: 14px 8px;
            border-radius: 25px;
          }

          .hx-visual {
            width: 118px;
            height: 118px;
            border-radius: 28px;
          }

          .hx-visual::before,
          .hx-visual::after {
            width: 150px;
          }

          .hx-visual ha-icon {
            width: 54px;
            height: 54px;
            --mdc-icon-size: 54px;
          }

          .core-value {
            font-size: 38px;
          }

          .core-unit {
            font-size: 15px;
          }

          .core-hint {
            font-size: 9px;
          }

          .core-meta-pill {
            min-height: 24px;
            padding: 3px 6px;
            font-size: 8px;
          }

          .boost-control {
            min-height: 35px;
            padding: 5px 9px;
            font-size: 10px;
          }

          .performance {
            padding: 13px;
          }

          .classic-mode {
            padding: 14px 11px;
          }

          .classic-mode-label,
          .classic-metric-label {
            font-size: 11px;
          }

          .classic-mode-value {
            font-size: 22px;
          }

          .classic-metric {
            grid-template-columns: 30px minmax(0, 1fr);
            min-height: 92px;
            padding: 11px 9px;
          }

          .classic-metric-icon {
            width: 28px;
            height: 28px;
          }

          .classic-metric-icon ha-icon {
            width: 18px;
            height: 18px;
            --mdc-icon-size: 18px;
          }

          .classic-metric-value {
            font-size: 20px;
          }

          .classic-metric-value small {
            font-size: 9px;
          }

          .classic-footer-item {
            padding: 12px 11px;
          }

          .classic-footer-item > span {
            font-size: 10px;
          }

          .classic-footer-item strong {
            font-size: 19px;
          }

          .classic-footer-item strong small {
            font-size: 9px;
          }
        }

        @media (max-width: 500px) {
          h2 {
            font-size: 19px;
          }

          .identity-icon {
            width: 45px;
            height: 45px;
            flex-basis: 45px;
            border-radius: 13px;
          }

          .statusbar {
            display: grid;
            grid-template-columns: 1fr 1fr;
            width: 100%;
          }

          .status-chip {
            justify-content: center;
            min-height: 44px;
            padding: 7px 8px;
            gap: 7px;
          }

          .status-chip ha-icon {
            width: 18px;
            height: 18px;
            --mdc-icon-size: 18px;
          }

          .chip-label {
            font-size: 9px;
          }

          .chip-value {
            font-size: 13px;
          }

          .bottom {
            grid-template-columns: 1fr 1fr;
          }
        }

        @media (prefers-reduced-motion: reduce) {
          *,
          *::before,
          *::after {
            animation-duration: .001ms !important;
            animation-iteration-count: 1 !important;
            transition-duration: .001ms !important;
          }
        }

        @keyframes spin {
          to {
            transform: rotate(360deg);
          }
        }

        @keyframes dash-forward {
          to {
            stroke-dashoffset: -50;
          }
        }

        @keyframes dash-reverse {
          to {
            stroke-dashoffset: 50;
          }
        }
      </style>

      <ha-card>
        <div class="shell">

          <header class="header">

            <div class="identity">
              <div class="identity-icon">
                <ha-icon icon="mdi:fan"></ha-icon>
              </div>

              <div>
                <h2 id="title">Zehnder ComfoAir Q</h2>

                <div class="subtitle">
                  <span>GerlCraft HVAC Bridge</span>

                  <span class="mini-pill">
                    Lüfter
                    <strong id="fan-level">–</strong>
                  </span>

                  <span class="mini-pill">
                    Modus
                    <strong id="operating-mode">–</strong>
                  </span>
                </div>
              </div>
            </div>

            <div class="statusbar">

              <div
                class="status-chip"
                id="connection-badge"
                data-key="connection"
              >
                <ha-icon icon="mdi:lan-connect"></ha-icon>
                <div class="chip-copy">
                  <span class="chip-label">Verbindung</span>
                  <strong class="chip-value" id="connection">–</strong>
                </div>
              </div>

              <div
                class="status-chip"
                id="bridge-badge"
                data-key="bridgeHealth"
              >
                <ha-icon icon="mdi:heart-pulse"></ha-icon>
                <div class="chip-copy">
                  <span class="chip-label">Bridge</span>
                  <strong class="chip-value" id="bridge-health">–</strong>
                </div>
              </div>

              <div
                class="status-chip"
                id="filter-badge"
                data-key="filterStatus"
              >
                <ha-icon icon="mdi:air-filter"></ha-icon>
                <div class="chip-copy">
                  <span class="chip-label">Filter</span>
                  <strong class="chip-value" id="filter-status">–</strong>
                </div>
              </div>

              <div
                class="status-chip muted"
                id="humidity-badge"
              >
                <ha-icon icon="mdi:water-percent"></ha-icon>
                <div class="chip-copy">
                  <span class="chip-label">Bad-Feuchte</span>
                  <strong class="chip-value" id="humidity">Nicht eingerichtet</strong>
                </div>
              </div>

            </div>

          </header>

          <div class="main">

            <section class="schematic">

              <div class="section-head">
                <div>
                  <div class="section-title">Airflow Signature</div>
                  <div class="section-sub">
                    Luftströme durch den Wärmetauscher
                  </div>
                </div>

                <div class="signature-badge">
                  <ha-icon icon="mdi:chart-timeline-variant-shimmer"></ha-icon>
                  Live Flow
                </div>
              </div>

              <div class="airspace">

                <svg
                  class="flow-svg"
                  viewBox="0 0 1000 520"
                  preserveAspectRatio="none"
                  aria-hidden="true"
                >
                  <!-- Außenluft -> Wärmetauscher -->
                  <path class="flow-base flow-cold" d="M150 135 C270 135 325 138 405 205"></path>
                  <path class="flow-edge flow-cold" d="M150 135 C270 135 325 138 405 205"></path>
                  <path class="flow-motion flow-cold forward" d="M150 135 C270 135 325 138 405 205"></path>

                  <!-- Wärmetauscher -> Zuluft -->
                  <path class="flow-base flow-cold" d="M595 205 C675 138 730 135 850 135"></path>
                  <path class="flow-edge flow-cold" d="M595 205 C675 138 730 135 850 135"></path>
                  <path class="flow-motion flow-cold forward" d="M595 205 C675 138 730 135 850 135"></path>

                  <!-- Abluft -> Wärmetauscher -->
                  <path class="flow-base flow-warm" d="M850 385 C730 385 675 382 595 315"></path>
                  <path class="flow-edge flow-warm" d="M850 385 C730 385 675 382 595 315"></path>
                  <path class="flow-motion flow-warm reverse" d="M850 385 C730 385 675 382 595 315"></path>

                  <!-- Wärmetauscher -> Fortluft -->
                  <path class="flow-base flow-hot" d="M405 315 C325 382 270 385 150 385"></path>
                  <path class="flow-edge flow-hot" d="M405 315 C325 382 270 385 150 385"></path>
                  <path class="flow-motion flow-hot reverse" d="M405 315 C325 382 270 385 150 385"></path>
                </svg>

                <div
                  class="node outdoor"
                  data-key="outdoorTemperature"
                >
                  <ha-icon icon="mdi:weather-windy"></ha-icon>
                  <span class="node-label">Außenluft</span>
                  <strong class="node-value">
                    <span id="outdoor-temperature">–</span> °C
                  </strong>
                  <span class="node-hint">von außen</span>
                </div>

                <div
                  class="node supply"
                  data-key="supplyTemperature"
                >
                  <ha-icon icon="mdi:home-import-outline"></ha-icon>
                  <span class="node-label">Zuluft</span>
                  <strong class="node-value">
                    <span id="supply-temperature">–</span> °C
                  </strong>
                  <span class="node-hint">ins Gebäude</span>
                </div>

                <div
                  class="node exhaust"
                  data-key="exhaustTemperature"
                >
                  <ha-icon icon="mdi:thermometer-chevron-down"></ha-icon>
                  <span class="node-label">Fortluft</span>
                  <strong class="node-value">
                    <span id="exhaust-temperature">–</span> °C
                  </strong>
                  <span class="node-hint">nach außen</span>
                </div>

                <div
                  class="node extract"
                  data-key="extractTemperature"
                >
                  <ha-icon icon="mdi:home-export-outline"></ha-icon>
                  <span class="node-label">Abluft</span>
                  <strong class="node-value">
                    <span id="extract-temperature">–</span> °C
                  </strong>
                  <span class="node-hint">aus dem Gebäude</span>
                </div>

                <div class="core">

                  <div class="hx-visual">
                    <span class="thermal-cold"></span>
                    <span class="thermal-warm"></span>
                    <ha-icon id="hero-icon" icon="mdi:fan"></ha-icon>
                  </div>

                  <div class="core-mode" id="hero-mode">
                    Anlagenstatus
                  </div>

                  <div class="core-number">
                    <strong class="core-value" id="hero-value">–</strong>
                    <span class="core-unit" id="hero-unit"></span>
                  </div>

                  <div class="core-hint" id="hero-hint">
                    Aktueller Anlagenzustand
                  </div>

                  <div class="core-meta">
                    <span class="core-meta-pill">
                      Bypass
                      <strong id="bypass-value">–</strong>
                    </span>

                    <span class="core-meta-pill">
                      Zuluft Δ
                      <strong id="temperature-lift">–</strong>
                    </span>
                  </div>

                  <button
                    class="boost-control"
                    id="boost-control"
                    data-action="toggle-boost"
                    type="button"
                    disabled
                  >
                    <ha-icon
                      id="boost-control-icon"
                      icon="mdi:fan-clock"
                    ></ha-icon>
                    <span id="boost-control-label">Boost starten</span>
                  </button>

                  <div class="boost-sub" id="boost-control-sub">
                    60-Minuten-Boost
                  </div>

                </div>

              </div>

            </section>

            <aside class="performance">

              <div class="section-head">
                <div>
                  <div class="section-title">Live-Betrieb</div>
                  <div class="section-sub">
                    Aktuelle Anlagenwerte
                  </div>
                </div>

                <span class="live">LIVE</span>
              </div>

              <div class="classic-mode-grid">

                <div class="classic-mode">
                  <span class="classic-mode-label">
                    Lüfterstufe
                  </span>

                  <strong
                    class="classic-mode-value"
                    id="fan-level-side"
                  >
                    –
                  </strong>
                </div>

                <div class="classic-mode">
                  <span class="classic-mode-label">
                    Betriebsmodus
                  </span>

                  <strong
                    class="classic-mode-value"
                    id="operating-mode-side"
                  >
                    –
                  </strong>
                </div>

              </div>

              <div class="classic-metric-grid">

                <div
                  class="classic-metric"
                  data-key="supplyFlow"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:arrow-collapse-down"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Zuluft
                  </span>

                  <strong class="classic-metric-value">
                    <span id="supply-flow">–</span>
                    <small>m³/h</small>
                  </strong>
                </div>

                <div
                  class="classic-metric"
                  data-key="extractFlow"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:arrow-expand-up"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Abluft
                  </span>

                  <strong class="classic-metric-value">
                    <span id="extract-flow">–</span>
                    <small>m³/h</small>
                  </strong>
                </div>

                <div
                  class="classic-metric"
                  data-key="airflowDeviation"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:scale-balance"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Abweichung
                  </span>

                  <strong class="classic-metric-value">
                    <span id="airflow-deviation">–</span>
                    <small>%</small>
                  </strong>
                </div>

                <div
                  class="classic-metric"
                  data-key="ventilationPower"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:flash"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Leistung
                  </span>

                  <strong class="classic-metric-value">
                    <span id="ventilation-power">–</span>
                    <small>W</small>
                  </strong>
                </div>

                <div
                  class="classic-metric"
                  data-key="supplySpeed"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:fan-chevron-up"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Zuluftdrehzahl
                  </span>

                  <strong class="classic-metric-value">
                    <span id="supply-speed">–</span>
                    <small>rpm</small>
                  </strong>
                </div>

                <div
                  class="classic-metric"
                  data-key="extractSpeed"
                >
                  <div class="classic-metric-icon">
                    <ha-icon icon="mdi:fan-chevron-down"></ha-icon>
                  </div>

                  <span class="classic-metric-label">
                    Abluftdrehzahl
                  </span>

                  <strong class="classic-metric-value">
                    <span id="extract-speed">–</span>
                    <small>rpm</small>
                  </strong>
                </div>

              </div>

              <div class="classic-footer-info">

                <div
                  class="classic-footer-item"
                  data-key="avoidedHeatingPower"
                >
                  <span>Aktuell eingespart</span>

                  <strong>
                    <span id="avoided-heating-power">–</span>
                    <small>W</small>
                  </strong>
                </div>

                <div class="classic-footer-item">
                  <span>Zuluft Δ</span>

                  <strong id="temperature-lift-side">
                    –
                  </strong>
                </div>

              </div>

            </aside>

          </div>

          <section class="bottom">

            <div class="bottom-item" data-key="recoveredHeatingEnergy">
              <div class="bottom-label">
                <ha-icon icon="mdi:heat-wave"></ha-icon>
                Heizenergie eingespart
              </div>
              <div class="bottom-value">
                <span id="recovered-heating-energy">–</span>
                <small>kWh</small>
              </div>
              <div class="bottom-hint">kumuliert</div>
            </div>

            <div class="bottom-item" data-key="ventilationEnergyYear">
              <div class="bottom-label">
                <ha-icon icon="mdi:flash-outline"></ha-icon>
                Lüftungsenergie
              </div>
              <div class="bottom-value">
                <span id="ventilation-energy">–</span>
                <small>kWh</small>
              </div>
              <div class="bottom-hint">laufendes Jahr</div>
            </div>

            <div class="bottom-item" id="filter-item" data-key="filterDays">
              <div class="bottom-label">
                <ha-icon icon="mdi:air-filter"></ha-icon>
                Filter
              </div>
              <div class="bottom-value">
                <span id="filter-days">–</span>
                <small>Tage</small>
              </div>
              <div class="bottom-hint" id="filter-hint">Restlaufzeit</div>
            </div>

            <div class="bottom-item">
              <div class="bottom-label">
                <ha-icon icon="mdi:call-split"></ha-icon>
                Anlagenmodus
              </div>
              <div class="bottom-value" id="free-cooling">
                –
              </div>
              <div class="bottom-hint" id="free-cooling-reason">
                –
              </div>
            </div>

          </section>

        </div>
      </ha-card>
`;
  }
}

if (!customElements.get(CARD_TYPE)) {
  customElements.define(
    CARD_TYPE,
    GerlCraftZehnderCardV14,
  );
}

window.customCards =
  window.customCards || [];

if (
  !window.customCards.some(
    (card) => card.type === CARD_TYPE,
  )
) {
  window.customCards.push({
    type: CARD_TYPE,
    name:
      "GerlCraft Zehnder Card v1.4 Signature",
    description:
      "Signature-Redesign für Zehnder ComfoAir Q",
    preview: true,
  });
}

console.info(
  `%c GERLCRAFT-ZEHNDER-CARD V1.4.5 FINAL %c v${CARD_VERSION} `,
  "color:white;background:#00897b;font-weight:700;padding:2px 6px;",
  "color:#00897b;background:transparent;font-weight:700;",
);
