
function demo_run()
%DEMO_RUN Seed workflow: parse STK Access reports and compute link margins.
%
% 1) Replace the sample CSVs in ../data with your STK exports (same filenames).
% 2) Adjust 'params' below.
% 3) Run demo_run.
%
clc; close all; clear;

proj = fileparts(mfilename('fullpath'));
dataDir = fullfile(proj, '..', 'data');
outDir  = fullfile(proj, '..', 'output');
if ~exist(outDir,'dir'), mkdir(outDir); end

% --------------------- Parameters ---------------------
params.link.freq_Hz        = 433e6;
params.link.bw_Hz          = 125e3;
params.link.sf             = 12;
params.link.rx_nf_dB       = 4.5;   % satellite RX NF
params.link.rx_gain_dBi    = 2.0;   % satellite RX antenna gain toward ground
params.link.tx_eirp_dBm    = 14.0;  % collar EIRP (adjust to your TX)
params.link.rx_sys_loss_dB = 1.0;   % feeder/cable/implementation
params.link.impl_margin_dB = 2.0;

params.gmsk.freq_Hz        = 145.9e6;
params.gmsk.rbps           = 9600;
params.gmsk.rx_nf_dB       = 3.0;   % GS RX NF
params.gmsk.rx_gain_dBi    = 12.0;  % GS antenna gain
params.gmsk.tx_eirp_dBm    = 27.0;  % sat VHF TX EIRP
params.gmsk.rx_sys_loss_dB = 1.0;
params.gmsk.bw_Hz          = [];    % default = Rb
params.gmsk.EbN0_req_dB    = 9.0;   % editable; depends on BER target/coding
params.gmsk.impl_margin_dB = 2.0;

% -------------------- Load STK reports ----------------
uplinkCSV   = fullfile(dataDir, 'access_lora_uplink_sample.csv');
downlinkCSV = fullfile(dataDir, 'access_gmsk_downlink_sample.csv');

accUL = parse_stk_access(uplinkCSV);
accDL = parse_stk_access(downlinkCSV);

% -------------------- Compute margins -----------------
loraOut = lora_sf_link_margin(accUL, params.link);
gmskOut = gmsk_link_margin(accDL, params.gmsk);

% -------------------- Save CSV summaries --------------
writetable(struct2table(loraOut), fullfile(outDir, 'lora_results.csv'));
writetable(struct2table(gmskOut), fullfile(outDir, 'gmsk_results.csv'));

% -------------------- Plots ---------------------------
figure; plot(loraOut.t_s, loraOut.margin_dB, 'LineWidth', 1.5);
grid on; xlabel('Time [s]'); ylabel('Margin [dB]'); title('LoRa Uplink Margin vs Time');
saveas(gcf, fullfile(outDir, 'lora_margin_vs_time.png'));

figure; plot(gmskOut.t_s, gmskOut.margin_dB, 'LineWidth', 1.5);
grid on; xlabel('Time [s]'); ylabel('Margin [dB]'); title('GMSK 9.6 kbps Downlink Margin vs Time');
saveas(gcf, fullfile(outDir, 'gmsk_margin_vs_time.png'));

disp('Done. See output/ for results.');
end
