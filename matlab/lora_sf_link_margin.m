
function out = lora_sf_link_margin(acc, params)
%LORA_SF_LINK_MARGIN Compute LoRa SNR and link margin over time.
%   out = lora_sf_link_margin(acc, params)
% Inputs:
%   acc    - struct from parse_stk_access()
%   params - struct with fields:
%       .freq_Hz        (e.g., 433e6)
%       .bw_Hz          (e.g., 125e3)
%       .sf             (7..12)
%       .rx_nf_dB       (noise figure) if acc.rx_nf_dB is NaN
%       .rx_gain_dBi    (fallback RX gain) if acc.rx_gain_dBi is NaN
%       .tx_eirp_dBm    (fallback EIRP) if acc.tx_eirp_dBm is NaN
%       .rx_sys_loss_dB (fallback RX system loss) if acc.rx_sys_loss_dB is NaN
%       .impl_margin_dB (implementation margin, default 0..3 dB)
%
% Output fields:
%   .t_s, .Pr_dBm, .N_dBm, .SNR_dB, .SNR_req_dB, .margin_dB
%
% Notes:
% - Required SNR per SF is approximated from Semtech app notes (typical values).
% - Margin = SNR - SNR_req - impl_margin_dB.
% - FSPL is read from STK report if present; otherwise computed from range & freq.
%
c = 299792458;
k = 1.38064852e-23;

f = params.freq_Hz;
BW = params.bw_Hz;
SF = params.sf;

% Required SNR (approximate) for BW=125 kHz
% Values in dB for SF7..SF12 (typical): [-7.5, -10, -12.5, -15, -17.5, -20]
snr_req_map = containers.Map({7,8,9,10,11,12}, [-7.5, -10, -12.5, -15, -17.5, -20]);
if ~isKey(snr_req_map, SF)
    error('SF must be an integer in 7..12');
end
SNR_req_dB = snr_req_map(SF);

% Fallbacks
rx_nf   = fillIfNaN(acc.rx_nf_dB, params.rx_nf_dB, 4); % dB
rx_gain = fillIfNaN(acc.rx_gain_dBi, params.rx_gain_dBi, 0); % dBi
eirp    = fillIfNaN(acc.tx_eirp_dBm, params.tx_eirp_dBm, 14); % dBm, e.g., collar
rx_sysL = fillIfNaN(acc.rx_sys_loss_dB, params.rx_sys_loss_dB, 1); % dB
implM   = defaultIfMissing(params, 'impl_margin_dB', 0);

% FSPL
lambda = c / f;
if all(isnan(acc.fspl_dB))
    % compute from range if available
    if all(isnan(acc.range_km))
        error('Neither FSPL nor Range found in access report');
    end
    R = acc.range_km * 1000; % m
    FSPL = 20*log10(4*pi*R/lambda);
else
    FSPL = acc.fspl_dB;
end

% Received power
Pr_dBm = eirp - FSPL - rx_sysL + rx_gain;

% Noise power
N_dBm = 10*log10(k*290*BW) + 30 + rx_nf; % dBW->dBm (+30)

% SNR
SNR_dB = Pr_dBm - N_dBm;

% Margin
margin_dB = SNR_dB - SNR_req_dB - implM;

out.t_s        = acc.t_s;
out.Pr_dBm     = Pr_dBm;
out.N_dBm      = N_dBm;
out.SNR_dB     = SNR_dB;
out.SNR_req_dB = SNR_req_dB*ones(size(SNR_dB));
out.margin_dB  = margin_dB;
end

function v = fillIfNaN(vec, fallback, defaultVal)
if nargin<3, defaultVal = NaN; end
if isempty(fallback), fallback = defaultVal; end
v = vec;
ix = isnan(v);
v(ix) = fallback;
end

function val = defaultIfMissing(s, field, defaultVal)
if ~isfield(s, field) || isempty(s.(field))
    val = defaultVal;
else
    val = s.(field);
end
end
