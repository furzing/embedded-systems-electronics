
function acc = parse_stk_access(filename)
%PARSE_STK_ACCESS Read an STK "Link Budget – Detailed" Access report (CSV/TSV).
%   acc = parse_stk_access(filename)
% Returns a struct with fields:
%   .t_s            - seconds from first timestamp
%   .utc            - datetime array (UTC)
%   .range_km       - slant range [km] if present
%   .elev_deg       - elevation angle [deg] if present
%   .fspl_dB        - free-space path loss [dB] if present
%   .rx_gain_dBi    - receiver antenna gain toward Tx [dBi] if present
%   .tx_eirp_dBm    - transmitter EIRP [dBm] if present
%   .rx_sys_loss_dB - receiver system losses [dB] if present
%   .rx_nf_dB       - receiver noise figure [dB] if present
%
% Notes:
% - STK headers vary depending on your style. We do a tolerant match.
% - Export your report with time-stamped rows and ensure Range/Elevation included.
%
% Author: seed project
opts = detectImportOptions(filename);
T = readtable(filename, opts);

% Try to map common columns
utcVar = firstMatch(T.Properties.VariableNames, {'Time', 'Date_Time', 'Epoch_UTC', 'Epoch'});
rngVar = firstMatch(T.Properties.VariableNames, {'Range', 'Range_km', 'Slant_Range'});
elVar  = firstMatch(T.Properties.VariableNames, {'Elevation', 'El', 'Elevation_Angle'});
fsplVar= firstMatch(T.Properties.VariableNames, {'Path_Loss', 'Free_Space_Path_Loss', 'FSPL'});
rxgVar = firstMatch(T.Properties.VariableNames, {'Receiver_Antenna_Gain', 'Rx_Antenna_Gain', 'Receiver_Gain'});
eirpVar= firstMatch(T.Properties.VariableNames, {'EIRP', 'Transmitter_EIRP'});
rxlossVar= firstMatch(T.Properties.VariableNames, {'Receiver_System_Loss', 'Receiver_Loss'});
nfigVar= firstMatch(T.Properties.VariableNames, {'Receiver_Noise_Figure', 'Noise_Figure'});

% Time
try
    utc = datetime(string(T.(utcVar)), 'InputFormat','yyyy-MM-dd HH:mm:ss.SSS', 'TimeZone','UTC');
catch
    try
        utc = datetime(T.(utcVar), 'TimeZone','UTC');
    catch
        error('Could not parse time column.');
    end
end
t0 = utc(1);
t_s = seconds(utc - t0);

acc.utc  = utc;
acc.t_s  = t_s;

% Numeric helpers
acc.range_km       = getIfExists(T, rngVar, 1e-3); % assume meters → km if large
acc.elev_deg       = getIfExists(T, elVar, 1.0);
acc.fspl_dB        = getIfExists(T, fsplVar, 1.0);
acc.rx_gain_dBi    = getIfExists(T, rxgVar, 1.0);
acc.tx_eirp_dBm    = getIfExists(T, eirpVar, 1.0);
acc.rx_sys_loss_dB = getIfExists(T, rxlossVar, 1.0);
acc.rx_nf_dB       = getIfExists(T, nfigVar, 1.0);

end

function name = firstMatch(names, candidates)
name = '';
for i=1:numel(candidates)
    ix = find(strcmpi(names, candidates{i}), 1, 'first');
    if ~isempty(ix), name = names{ix}; return; end
end
% fuzzy find
for i=1:numel(names)
    for j=1:numel(candidates)
        if contains(lower(names{i}), lower(candidates{j}))
            name = names{i}; return;
        end
    end
end
end

function v = getIfExists(T, varName, scale)
if isempty(varName) || ~ismember(varName, T.Properties.VariableNames)
    v = NaN(height(T),1);
else
    v = T.(varName);
    v = v .* scale;
end
end
