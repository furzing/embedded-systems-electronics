
function out = gmsk_link_margin(acc, params)
%GMSK_LINK_MARGIN Compute GMSK SNR, Eb/N0, and link margin versus time.
%   out = gmsk_link_margin(acc, params)
% Inputs:
%   acc    - struct from parse_stk_access()
%   params - struct with fields:
%       .freq_Hz
%       .rbps           - bit rate [bits/s], e.g., 9600
%       .rx_nf_dB
%       .rx_gain_dBi
%       .tx_eirp_dBm
%       .rx_sys_loss_dB
%       .bw_Hz          - receiver noise bandwidth; if empty, set ~= rbps
%       .EbN0_req_dB    - target required Eb/N0 for desired BER (e.g., 9 dB for uncoded ~1e-5)
%       .impl_margin_dB - additional margin
%
% Output:
%   .t_s, .Pr_dBm, .N_dBm, .SNR_dB, .EbN0_dB, .margin_dB
%
k = 1.38064852e-23;
f = params.freq_Hz;
Rb = params.rbps;
BW = params.bw_Hz;
if isempty(BW) || BW<=0, BW = Rb; end

rx_nf   = fillIfNaN(acc.rx_nf_dB, params.rx_nf_dB, 3);
rx_gain = fillIfNaN(acc.rx_gain_dBi, params.rx_gain_dBi, 8);
eirp    = fillIfNaN(acc.tx_eirp_dBm, params.tx_eirp_dBm, 27);
rx_sysL = fillIfNaN(acc.rx_sys_loss_dB, params.rx_sys_loss_dB, 1);
EbN0req = params.EbN0_req_dB;
implM   = defaultIfMissing(params, 'impl_margin_dB', 0);

% FSPL
if all(isnan(acc.fspl_dB))
    if all(isnan(acc.range_km))
        error('Neither FSPL nor Range found in access report');
    end
    R = acc.range_km * 1000; % m
    lambda = 299792458 / f;
    FSPL = 20*log10(4*pi*R/lambda);
else
    FSPL = acc.fspl_dB;
end

Pr_dBm = eirp - FSPL - rx_sysL + rx_gain;
N_dBm  = 10*log10(k*290*BW) + 30 + rx_nf; % dBm
SNR_dB = Pr_dBm - N_dBm;

% Relate SNR to Eb/N0: Eb/N0 = SNR * (BW/Rb)
EbN0_dB = SNR_dB + 10*log10(BW/Rb);

margin_dB = EbN0_dB - EbN0req - implM;

out.t_s       = acc.t_s;
out.Pr_dBm    = Pr_dBm;
out.N_dBm     = N_dBm;
out.SNR_dB    = SNR_dB;
out.EbN0_dB   = EbN0_dB;
out.margin_dB = margin_dB;
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
