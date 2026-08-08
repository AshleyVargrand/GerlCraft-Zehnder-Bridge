const CARD_VERSION = "1.0.2";
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
  supplyFlow: { domain: "sensor", suffixes: ["zuluftvolumenstrom"] },
  extractFlow: { domain: "sensor", suffixes: ["abluftvolumenstrom"] },
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
    suffixes: ["eingesparte_heizenergie_gesamt"],
  },
};

const INVALID_STATES = new Set(["", "unknown", "unavailable", "none", "null"]);

class GerlCraftZehnderCard extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    this._config = {};
    this._hass = undefined;
    this._prefix = DEFAULT_PREFIX;
    this._renderShell();
    this.shadowRoot.addEventListener("click", (event) => this._handleClick(event));
  }

  static getStubConfig() {
    return {
      title: "Zehnder ComfoAir Q",
    };
  }

  setConfig(config) {
    this._config = {
      ...config,
      entities: { ...(config.entities || {}) },
    };
    this._setText("title", this._config.title || "Zehnder ComfoAir Q");
    this._update();
  }

  set hass(hass) {
    this._hass = hass;
    this._prefix = this._detectPrefix();
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
      (entityId) => entityId.startsWith("sensor.") && entityId.endsWith(suffix),
    );

    if (!match) {
      return DEFAULT_PREFIX;
    }

    return match.slice("sensor.".length, -suffix.length);
  }

  _entityId(key) {
    const configured = this._config.entities?.[key];

    if (configured) {
      return configured;
    }

    const definition = ENTITY_DEFINITIONS[key];

    if (!definition) {
      return undefined;
    }

    for (const suffix of definition.suffixes) {
      const entityId = `${definition.domain}.${this._prefix}_${suffix}`;

      if (this._hass?.states?.[entityId]) {
        return entityId;
      }
    }

    return `${definition.domain}.${this._prefix}_${definition.suffixes[0]}`;
  }

  _state(key) {
    const entityId = this._entityId(key);
    return entityId ? this._hass?.states?.[entityId] : undefined;
  }

  _isValid(stateObject) {
    return Boolean(
      stateObject && !INVALID_STATES.has(String(stateObject.state).toLowerCase()),
    );
  }

  _value(key, digits = 0) {
    const stateObject = this._state(key);

    if (!this._isValid(stateObject)) {
      return "–";
    }

    const numericValue = Number(stateObject.state);

    if (!Number.isFinite(numericValue)) {
      return stateObject.state;
    }

    const language = this._hass?.locale?.language || "de-DE";

    return numericValue.toLocaleString(language, {
      minimumFractionDigits: digits,
      maximumFractionDigits: digits,
    });
  }

  _rawNumber(key) {
    const stateObject = this._state(key);

    if (!this._isValid(stateObject)) {
      return undefined;
    }

    const value = Number(stateObject.state);
    return Number.isFinite(value) ? value : undefined;
  }

  _setText(id, value) {
    const element = this.shadowRoot.getElementById(id);

    if (element) {
      element.textContent = value;
    }
  }

  _setMetric(id, key, digits = 0) {
    this._setText(id, this._value(key, digits));
    const element = this.shadowRoot.getElementById(id)?.closest("[data-key]");

    if (element) {
      element.dataset.key = key;
      element.classList.toggle("unavailable", !this._isValid(this._state(key)));
    }
  }

  _update() {
    if (!this._hass) {
      return;
    }

    const connected = this._state("connection")?.state === "on";
    const freeCooling = this._state("freeCooling")?.state === "on";
    const bridgeHealth = this._value("bridgeHealth");
    const filterStatus = this._value("filterStatus");
    const bypass = this._rawNumber("bypass") || 0;
    const supplyFlow = this._rawNumber("supplyFlow") || 0;
    const extractFlow = this._rawNumber("extractFlow") || 0;
    const supplySpeed = this._rawNumber("supplySpeed") || 0;
    const extractSpeed = this._rawNumber("extractSpeed") || 0;
    const averageFlow = (supplyFlow + extractFlow) / 2;
    const averageSpeed = (supplySpeed + extractSpeed) / 2;

    const card = this.shadowRoot.querySelector("ha-card");
    card.classList.toggle("connected", connected);
    card.classList.toggle("disconnected", !connected);
    card.classList.toggle("free-cooling", freeCooling);
    card.style.setProperty(
      "--flow-duration",
      `${Math.max(1.1, Math.min(3.8, 4.2 - averageFlow / 110)).toFixed(2)}s`,
    );
    card.style.setProperty(
      "--fan-duration",
      `${Math.max(0.8, Math.min(4, 4.5 - averageSpeed / 650)).toFixed(2)}s`,
    );
    card.style.setProperty("--bypass-angle", `${Math.min(90, bypass * 0.9)}deg`);

    this._setText("connection", connected ? "Verbunden" : "Getrennt");
    this._setText("fan-level", this._value("fanLevel"));
    this._setText("operating-mode", this._value("operatingMode"));
    this._setText("bridge-health", bridgeHealth);
    this._setText("filter-status", filterStatus);
    this._setText("free-cooling", "Freie Kühlung");
    this._setText("free-cooling-reason", this._value("freeCoolingReason"));
    this._setText("outdoor-temperature", this._value("outdoorTemperature", 1));
    this._setText("supply-temperature", this._value("supplyTemperature", 1));
    this._setText("extract-temperature", this._value("extractTemperature", 1));
    this._setText("exhaust-temperature", this._value("exhaustTemperature", 1));

    this._setMetric("supply-flow", "supplyFlow");
    this._setMetric("extract-flow", "extractFlow");
    this._setMetric("airflow-deviation", "airflowDeviation", 1);
    this._setMetric("supply-speed", "supplySpeed");
    this._setMetric("extract-speed", "extractSpeed");
    this._setMetric("ventilation-power", "ventilationPower");
    this._setMetric("filter-days", "filterDays");
    this._setMetric("ventilation-energy", "ventilationEnergyYear");
    this._setMetric("recovered-heating-energy", "recoveredHeatingEnergy");
    this._setText("bypass", this._value("bypass"));
    this._setMetric("bypass-footer", "bypass");

    this._updateRecovery("heat-recovery", "heatRecovery");
    this._updateRecovery("cooling-recovery", "coolingRecovery");

    const healthBadge = this.shadowRoot.getElementById("bridge-badge");
    healthBadge.classList.toggle("ok", bridgeHealth === "OK");
    healthBadge.classList.toggle("alert", bridgeHealth !== "OK");

    const filterBadge = this.shadowRoot.getElementById("filter-badge");
    filterBadge.classList.toggle("ok", filterStatus === "OK");
    filterBadge.classList.toggle("alert", filterStatus !== "OK");
  }

  _updateRecovery(id, key) {
    const stateObject = this._state(key);
    const valid = this._isValid(stateObject);
    const element = this.shadowRoot.getElementById(id);
    const container = element?.closest("[data-key]");

    if (!element || !container) {
      return;
    }

    element.textContent = valid ? this._value(key, 1) : "–";
    container.dataset.key = key;
    container.classList.toggle("unavailable", !valid);
    const hint = container.querySelector(".metric-hint");

    if (hint) {
      hint.textContent = valid ? "Aktueller Wirkungsgrad" : "Derzeit nicht aktiv";
    }
  }

  _handleClick(event) {
    const target = event.target.closest("[data-key]");

    if (!target || !this._hass) {
      return;
    }

    const entityId = this._entityId(target.dataset.key);

    if (!entityId || !this._hass.states[entityId]) {
      return;
    }

    this.dispatchEvent(
      new CustomEvent("hass-more-info", {
        bubbles: true,
        composed: true,
        detail: { entityId },
      }),
    );
  }

  _renderShell() {
    this.shadowRoot.innerHTML = `
      <style>
        :host {
          display: block;
          --card-accent: #00a99a;
          --supply-color: #21a7e0;
          --extract-color: #f6a21a;
          --exhaust-color: #ef6c45;
          --flow-duration: 2s;
          --fan-duration: 2s;
          --bypass-angle: 0deg;
        }

        * { box-sizing: border-box; }

        ha-card {
          display: block;
          width: 100%;
          min-height: calc(100dvh - 56px);
          margin: 0;
          border-radius: 0;
          overflow: hidden;
          background: var(--ha-card-background, var(--card-background-color));
          color: var(--primary-text-color);
          box-shadow: var(--ha-card-box-shadow, none);
        }

        button {
          color: inherit;
          font: inherit;
        }

        .shell {
          display: flex;
          flex-direction: column;
          min-height: calc(100dvh - 56px);
          padding: 22px;
        }

        .header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 20px;
          padding-bottom: 18px;
          border-bottom: 1px solid var(--divider-color);
        }

        .identity {
          display: grid;
          grid-template-columns: 52px 1fr;
          grid-template-areas: "icon title" "icon subtitle";
          align-items: center;
          column-gap: 14px;
          min-width: 0;
        }

        .identity .fan-wrap {
          grid-area: icon;
          display: grid;
          place-items: center;
          width: 52px;
          height: 52px;
          border-radius: 50%;
          color: var(--card-accent);
          background: color-mix(in srgb, var(--card-accent) 13%, transparent);
        }

        .connected .fan-wrap ha-icon { animation: fan-spin var(--fan-duration) linear infinite; }

        h2 {
          grid-area: title;
          margin: 0;
          overflow: hidden;
          font-size: 21px;
          font-weight: 650;
          letter-spacing: 0;
          text-overflow: ellipsis;
          white-space: nowrap;
        }

        .subtitle {
          grid-area: subtitle;
          color: var(--secondary-text-color);
          font-size: 12px;
        }

        .badges { display: flex; flex-wrap: wrap; justify-content: flex-end; gap: 8px; }

        .badge {
          display: inline-flex;
          align-items: center;
          gap: 8px;
          min-height: 42px;
          padding: 5px 12px;
          border: 1px solid var(--divider-color);
          border-radius: 6px;
          color: var(--secondary-text-color);
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
          white-space: nowrap;
        }

        .badge ha-icon { width: 18px; }
        .badge-copy { display: grid; gap: 1px; line-height: 1.1; }
        .badge-label {
          color: var(--secondary-text-color);
          font-size: 9px;
          font-weight: 500;
          text-transform: uppercase;
        }
        .badge-value { font-size: 12px; font-weight: 650; }
        .connected #connection-badge { color: var(--success-color); }
        .disconnected #connection-badge { color: var(--error-color); }
        .badge.ok { color: var(--success-color); }
        .badge.alert { color: var(--warning-color); }

        .overview {
          display: grid;
          grid-template-columns: minmax(0, 1.55fr) minmax(310px, 0.75fr);
          flex: 1;
          gap: 22px;
          padding: 22px 0;
        }

        .overview > section {
          display: flex;
          flex-direction: column;
          min-height: 0;
        }

        .section-label {
          margin: 0 0 13px;
          color: var(--secondary-text-color);
          font-size: 11px;
          font-weight: 650;
          letter-spacing: 0;
          text-transform: uppercase;
        }

        .airflow-stage {
          display: grid;
          grid-template-columns: 112px minmax(42px, 1fr) 130px minmax(42px, 1fr) 112px;
          grid-template-areas:
            "outdoor intake unit supply-duct supply"
            "exhaust exhaust-duct unit extract-duct extract";
          grid-template-rows: 1fr 1fr;
          flex: 1;
          gap: 22px 10px;
          min-height: 330px;
          padding: 22px 16px;
          border-top: 1px solid var(--divider-color);
          border-bottom: 1px solid var(--divider-color);
        }

        .air-node {
          display: grid;
          align-content: center;
          justify-items: center;
          gap: 5px;
          min-width: 0;
          padding: 12px 6px;
          text-align: center;
        }

        .air-node ha-icon { width: 29px; color: var(--secondary-text-color); }
        .air-node .node-label { color: var(--secondary-text-color); font-size: 11px; }
        .air-node .node-value { font-size: 20px; font-weight: 650; white-space: nowrap; }
        .air-node .unit { color: var(--secondary-text-color); font-size: 10px; }
        .outdoor { grid-area: outdoor; }
        .supply { grid-area: supply; }
        .extract { grid-area: extract; }
        .exhaust { grid-area: exhaust; }
        .outdoor ha-icon, .supply ha-icon { color: var(--supply-color); }
        .extract ha-icon { color: var(--extract-color); }
        .exhaust ha-icon { color: var(--exhaust-color); }

        .duct {
          position: relative;
          align-self: center;
          height: 28px;
          overflow: hidden;
        }

        .duct::before {
          content: "";
          position: absolute;
          top: 13px;
          right: 0;
          left: 0;
          height: 2px;
          background: color-mix(in srgb, currentColor 34%, transparent);
        }

        .duct span {
          position: absolute;
          top: 9px;
          left: -8px;
          width: 9px;
          height: 9px;
          border-radius: 50%;
          background: currentColor;
          opacity: 0;
        }

        .connected .duct span { animation: particle-right var(--flow-duration) linear infinite; }
        .connected .duct.reverse span { animation-name: particle-left; }
        .duct span:nth-child(2) { animation-delay: calc(var(--flow-duration) * -0.33); }
        .duct span:nth-child(3) { animation-delay: calc(var(--flow-duration) * -0.66); }
        .intake { grid-area: intake; color: var(--supply-color); }
        .supply-duct { grid-area: supply-duct; color: var(--supply-color); }
        .extract-duct { grid-area: extract-duct; color: var(--extract-color); }
        .exhaust-duct { grid-area: exhaust-duct; color: var(--exhaust-color); }

        .unit-core {
          grid-area: unit;
          position: relative;
          display: grid;
          align-self: stretch;
          align-content: center;
          justify-items: center;
          gap: 7px;
          min-height: 236px;
          padding: 18px 10px;
          border: 1px solid var(--divider-color);
          border-radius: 8px;
          background: color-mix(in srgb, var(--primary-text-color) 3%, transparent);
        }

        .exchanger {
          position: relative;
          display: grid;
          place-items: center;
          width: 80px;
          height: 80px;
          border: 1px solid color-mix(in srgb, var(--card-accent) 48%, transparent);
          border-radius: 8px;
          overflow: hidden;
          color: var(--card-accent);
        }

        .exchanger::before,
        .exchanger::after {
          content: "";
          position: absolute;
          width: 110px;
          height: 1px;
          background: color-mix(in srgb, var(--card-accent) 38%, transparent);
          transform: rotate(45deg);
        }

        .exchanger::after { transform: rotate(-45deg); }
        .exchanger ha-icon { z-index: 1; width: 38px; }
        .connected .exchanger ha-icon { animation: fan-spin var(--fan-duration) linear infinite; }
        .unit-name { font-size: 11px; font-weight: 650; }
        .unit-mode { color: var(--secondary-text-color); font-size: 10px; text-align: center; }
        .free-cooling .unit-mode { color: var(--info-color); }

        .bypass-track {
          position: relative;
          width: 74px;
          height: 12px;
          margin-top: 7px;
          border-top: 2px solid var(--warning-color);
          opacity: 0.8;
        }

        .bypass-gate {
          position: absolute;
          top: -5px;
          left: 34px;
          width: 2px;
          height: 12px;
          background: var(--warning-color);
          transform: rotate(var(--bypass-angle));
          transform-origin: bottom center;
          transition: transform 0.5s ease;
        }

        .bypass-label { color: var(--secondary-text-color); font-size: 9px; }

        .operation {
          display: grid;
          grid-template-rows: auto 1fr;
          align-content: stretch;
          gap: 18px;
          min-width: 0;
        }

        .mode-panel {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: 1px;
          overflow: hidden;
          border: 1px solid var(--divider-color);
          border-radius: 8px;
          background: var(--divider-color);
        }

        .mode-value {
          display: grid;
          gap: 5px;
          padding: 17px;
          background: var(--ha-card-background, var(--card-background-color));
        }

        .mode-value span { color: var(--secondary-text-color); font-size: 10px; }
        .mode-value strong { font-size: 21px; font-weight: 650; }

        .metric-grid {
          display: grid;
          grid-template-columns: repeat(2, minmax(0, 1fr));
          grid-auto-rows: 1fr;
          border-top: 1px solid var(--divider-color);
        }

        .metric {
          display: grid;
          grid-template-columns: 30px 1fr;
          grid-template-areas: "icon label" "icon value";
          align-items: center;
          min-height: 78px;
          padding: 13px 11px;
          border-right: 1px solid var(--divider-color);
          border-bottom: 1px solid var(--divider-color);
          border-left: 0;
          border-top: 0;
          background: transparent;
          text-align: left;
          cursor: pointer;
        }

        .metric:nth-child(even) { border-right: 0; }
        .metric:hover { background: color-mix(in srgb, var(--primary-text-color) 4%, transparent); }
        .metric ha-icon { grid-area: icon; width: 22px; color: var(--secondary-text-color); }
        .metric-label { grid-area: label; color: var(--secondary-text-color); font-size: 10px; }
        .metric-reading { grid-area: value; font-size: 17px; font-weight: 650; white-space: nowrap; }
        .metric-reading small { margin-left: 3px; color: var(--secondary-text-color); font-size: 9px; font-weight: 500; }
        .metric.unavailable .metric-reading { color: var(--disabled-text-color); }
        .metric.unavailable small { display: none; }

        .footer-grid {
          display: grid;
          grid-template-columns: repeat(3, minmax(0, 1fr));
          gap: 1px;
          overflow: hidden;
          border: 1px solid var(--divider-color);
          border-radius: 8px;
          background: var(--divider-color);
        }

        .footer-panel {
          display: grid;
          align-content: start;
          gap: 14px;
          min-width: 0;
          padding: 17px;
          background: var(--ha-card-background, var(--card-background-color));
        }

        .footer-title {
          display: flex;
          align-items: center;
          gap: 8px;
          color: var(--secondary-text-color);
          font-size: 11px;
          font-weight: 650;
        }

        .footer-title ha-icon { width: 19px; }
        .footer-values { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 13px; }
        .footer-value { display: grid; gap: 3px; min-width: 0; cursor: pointer; }
        .footer-value span { color: var(--secondary-text-color); font-size: 9px; }
        .footer-value strong { overflow: hidden; font-size: 16px; font-weight: 650; text-overflow: ellipsis; white-space: nowrap; }
        .footer-value strong small { margin-left: 3px; color: var(--secondary-text-color); font-size: 9px; font-weight: 500; }
        .metric-hint { color: var(--secondary-text-color); font-size: 9px; }
        .footer-value.unavailable strong { color: var(--disabled-text-color); }
        .footer-value.unavailable strong small { display: none; }

        @keyframes fan-spin { to { transform: rotate(360deg); } }
        @keyframes particle-right {
          0% { left: -8px; opacity: 0; }
          12%, 88% { opacity: 0.9; }
          100% { left: calc(100% + 2px); opacity: 0; }
        }
        @keyframes particle-left {
          0% { left: calc(100% + 2px); opacity: 0; }
          12%, 88% { opacity: 0.9; }
          100% { left: -8px; opacity: 0; }
        }

        @media (max-width: 900px) {
          ha-card { width: 100%; margin: 0; }
          .overview { grid-template-columns: 1fr; }
          .footer-grid { grid-template-columns: 1fr; }
        }

        @media (max-width: 620px) {
          .shell { padding: 15px; }
          .header { flex-direction: column; align-items: stretch; gap: 12px; }
          .badges {
            display: grid;
            grid-template-columns: minmax(0, 1.25fr) repeat(2, minmax(0, 1fr));
            width: 100%;
            gap: 6px;
          }
          .badge { justify-content: center; min-width: 0; padding-inline: 6px; }
          .identity { grid-template-columns: 44px 1fr; column-gap: 10px; }
          .identity .fan-wrap { width: 44px; height: 44px; }
          h2 {
            overflow: visible;
            font-size: 16px;
            line-height: 1.15;
            white-space: normal;
          }
          .airflow-stage {
            grid-template-columns: 68px minmax(20px, 1fr) 82px minmax(20px, 1fr) 68px;
            min-height: 285px;
            padding: 15px 2px;
            column-gap: 3px;
          }
          .air-node { padding: 8px 1px; }
          .air-node .node-value { font-size: 14px; }
          .air-node .node-label { font-size: 9px; }
          .air-node .unit { display: none; }
          .unit-core { min-height: 210px; padding: 12px 4px; }
          .exchanger { width: 58px; height: 58px; }
          .unit-name { display: none; }
          .metric-grid { grid-template-columns: 1fr 1fr; }
          .metric { min-height: 72px; padding: 10px 7px; }
          .metric-reading { font-size: 15px; }
          .footer-values { grid-template-columns: 1fr 1fr; }
        }

        @media (prefers-reduced-motion: reduce) {
          *, *::before, *::after {
            animation-duration: 0.001ms !important;
            animation-iteration-count: 1 !important;
            transition-duration: 0.001ms !important;
          }
        }
      </style>

      <ha-card>
        <div class="shell">
          <header class="header">
            <div class="identity">
              <div class="fan-wrap"><ha-icon icon="mdi:fan"></ha-icon></div>
              <h2 id="title">Zehnder ComfoAir Q</h2>
              <div class="subtitle">GerlCraft HVAC Bridge</div>
            </div>
            <div class="badges">
              <span class="badge" id="connection-badge">
                <ha-icon icon="mdi:lan-connect"></ha-icon>
                <span class="badge-copy"><span class="badge-label">Verbindung</span><strong class="badge-value" id="connection">–</strong></span>
              </span>
              <span class="badge" id="bridge-badge">
                <ha-icon icon="mdi:heart-pulse"></ha-icon>
                <span class="badge-copy"><span class="badge-label">Bridge</span><strong class="badge-value" id="bridge-health">–</strong></span>
              </span>
              <span class="badge" id="filter-badge">
                <ha-icon icon="mdi:air-filter"></ha-icon>
                <span class="badge-copy"><span class="badge-label">Filter</span><strong class="badge-value" id="filter-status">–</strong></span>
              </span>
            </div>
          </header>

          <div class="overview">
            <section>
              <h3 class="section-label">Luftführung</h3>
              <div class="airflow-stage">
                <div class="air-node outdoor" data-key="outdoorTemperature">
                  <ha-icon icon="mdi:weather-windy"></ha-icon>
                  <span class="node-label">Außenluft</span>
                  <strong class="node-value"><span id="outdoor-temperature">–</span> °C</strong>
                  <span class="unit">von außen</span>
                </div>
                <div class="duct intake"><span></span><span></span><span></span></div>
                <div class="unit-core">
                  <div class="exchanger"><ha-icon icon="mdi:fan"></ha-icon></div>
                  <span class="unit-name">Wärmetauscher</span>
                  <span class="unit-mode"><span id="free-cooling">–</span> · <span id="free-cooling-reason">–</span></span>
                  <div class="bypass-track"><i class="bypass-gate"></i></div>
                  <span class="bypass-label">Bypass <span id="bypass">–</span> %</span>
                </div>
                <div class="duct supply-duct"><span></span><span></span><span></span></div>
                <div class="air-node supply" data-key="supplyTemperature">
                  <ha-icon icon="mdi:home-import-outline"></ha-icon>
                  <span class="node-label">Zuluft</span>
                  <strong class="node-value"><span id="supply-temperature">–</span> °C</strong>
                  <span class="unit">ins Gebäude</span>
                </div>
                <div class="air-node exhaust" data-key="exhaustTemperature">
                  <ha-icon icon="mdi:thermometer-chevron-down"></ha-icon>
                  <span class="node-label">Fortluft</span>
                  <strong class="node-value"><span id="exhaust-temperature">–</span> °C</strong>
                  <span class="unit">nach außen</span>
                </div>
                <div class="duct exhaust-duct reverse"><span></span><span></span><span></span></div>
                <div class="duct extract-duct reverse"><span></span><span></span><span></span></div>
                <div class="air-node extract" data-key="extractTemperature">
                  <ha-icon icon="mdi:home-export-outline"></ha-icon>
                  <span class="node-label">Abluft</span>
                  <strong class="node-value"><span id="extract-temperature">–</span> °C</strong>
                  <span class="unit">aus dem Gebäude</span>
                </div>
              </div>
            </section>

            <aside class="operation">
              <div>
                <h3 class="section-label">Betrieb</h3>
                <div class="mode-panel">
                  <div class="mode-value"><span>Lüfterstufe</span><strong id="fan-level">–</strong></div>
                  <div class="mode-value"><span>Betriebsmodus</span><strong id="operating-mode">–</strong></div>
                </div>
              </div>
              <div class="metric-grid">
                <button class="metric" data-key="supplyFlow"><ha-icon icon="mdi:arrow-collapse-down"></ha-icon><span class="metric-label">Zuluft</span><strong class="metric-reading"><span id="supply-flow">–</span><small>m³/h</small></strong></button>
                <button class="metric" data-key="extractFlow"><ha-icon icon="mdi:arrow-expand-up"></ha-icon><span class="metric-label">Abluft</span><strong class="metric-reading"><span id="extract-flow">–</span><small>m³/h</small></strong></button>
                <button class="metric" data-key="airflowDeviation"><ha-icon icon="mdi:scale-balance"></ha-icon><span class="metric-label">Abweichung</span><strong class="metric-reading"><span id="airflow-deviation">–</span><small>%</small></strong></button>
                <button class="metric" data-key="ventilationPower"><ha-icon icon="mdi:flash"></ha-icon><span class="metric-label">Leistung</span><strong class="metric-reading"><span id="ventilation-power">–</span><small>W</small></strong></button>
                <button class="metric" data-key="supplySpeed"><ha-icon icon="mdi:fan-chevron-up"></ha-icon><span class="metric-label">Zuluftdrehzahl</span><strong class="metric-reading"><span id="supply-speed">–</span><small>rpm</small></strong></button>
                <button class="metric" data-key="extractSpeed"><ha-icon icon="mdi:fan-chevron-down"></ha-icon><span class="metric-label">Abluftdrehzahl</span><strong class="metric-reading"><span id="extract-speed">–</span><small>rpm</small></strong></button>
              </div>
            </aside>
          </div>

          <div class="footer-grid">
            <section class="footer-panel">
              <div class="footer-title"><ha-icon icon="mdi:recycle-variant"></ha-icon>Rückgewinnung</div>
              <div class="footer-values">
                <div class="footer-value" data-key="heatRecovery"><span>Wärme</span><strong><span id="heat-recovery">–</span><small>%</small></strong><em class="metric-hint">Derzeit nicht aktiv</em></div>
                <div class="footer-value" data-key="coolingRecovery"><span>Kälte</span><strong><span id="cooling-recovery">–</span><small>%</small></strong><em class="metric-hint">Derzeit nicht aktiv</em></div>
              </div>
            </section>
            <section class="footer-panel">
              <div class="footer-title"><ha-icon icon="mdi:tools"></ha-icon>Wartung</div>
              <div class="footer-values">
                <div class="footer-value" data-key="filterDays"><span>Filterrestlaufzeit</span><strong><span id="filter-days">–</span><small>Tage</small></strong></div>
                <div class="footer-value" data-key="bypass"><span>Bypass</span><strong><span id="bypass-footer">–</span><small>%</small></strong></div>
              </div>
            </section>
            <section class="footer-panel">
              <div class="footer-title"><ha-icon icon="mdi:chart-line"></ha-icon>Energie</div>
              <div class="footer-values">
                <div class="footer-value" data-key="ventilationEnergyYear"><span>Lüftung im Jahr</span><strong><span id="ventilation-energy">–</span><small>kWh</small></strong></div>
                <div class="footer-value" data-key="recoveredHeatingEnergy"><span>Heizenergie zurückgewonnen</span><strong><span id="recovered-heating-energy">–</span><small>kWh</small></strong></div>
              </div>
            </section>
          </div>
        </div>
      </ha-card>
    `;
  }
}

if (!customElements.get("gerlcraft-zehnder-card")) {
  customElements.define("gerlcraft-zehnder-card", GerlCraftZehnderCard);
}

window.customCards = window.customCards || [];

if (!window.customCards.some((card) => card.type === "gerlcraft-zehnder-card")) {
  window.customCards.push({
    type: "gerlcraft-zehnder-card",
    name: "GerlCraft Zehnder Card",
    description: "Animierte Anlagenvisualisierung für Zehnder ComfoAir Q",
    preview: true,
    documentationURL:
      "https://github.com/AshleyVargrand/GerlCraft-Zehnder-Bridge/tree/main/home-assistant/custom-card",
  });
}

console.info(
  `%c GERLCRAFT-ZEHNDER-CARD %c v${CARD_VERSION} `,
  "color: white; background: #00897b; font-weight: 700; padding: 2px 6px;",
  "color: #00897b; background: transparent; font-weight: 700;",
);
