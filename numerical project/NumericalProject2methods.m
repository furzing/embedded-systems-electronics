% rf_stub_match_distance_two_methods.m
% RF Nonlinear Equation: Find distance d where Re{y_in(d)} = 1
% for single shunt stub matching on a lossless transmission line.
%
% Two numerical methods:
% 1) Bisection
% 2) Newton-Raphson (derivative via central difference, still Newton update)
%
% No built in solvers (fzero, etc.) are used!

clear; clc;

%% Given RF Parameters 
Z0 = 50;                 % Ohms
ZL = 30 - 1j*20;         % Ohms (example complex load)
f  = 2.4e9;              % Hz
c  = 3e8;                % m/s
lambda = c/f;            % m
beta = 2*pi/lambda;      % rad/m

% Search domain for d: [0, lambda/2) covers one unique matching region
d_min = 0;
d_max = 0.5*lambda;

% Stopping settings
tol = 1e-10;
maxIter = 200;

fprintf("RF Nonlinear Solver: Find d such that Re{y_in(d)} = 1\n");
fprintf("Z0=%.2f Ohm, ZL=%.2f%+.2fj Ohm, f=%.3f GHz\n", Z0, real(ZL), imag(ZL), f/1e9);
fprintf("lambda = %.6f m\n\n", lambda);

%% here we define the nonlinear function F(d) 
% F(d) = Re{y_in(d)} - 1 = 0

F = @(d) real( y_in_norm(d, Z0, ZL, beta) ) - 1;

% this is a looked up helper to avoid singularities where tan(beta*d) blows up
isBad = @(d) abs(cos(beta*d)) < 1e-6;   % cos ~ 0 => tan huge

%%  auto find a valid bracket for Bisection 
Nscan = 4000;
ds = linspace(d_min, d_max, Nscan);

a = NaN; b = NaN;
Fa_prev = NaN;
d_prev = NaN;

for k = 1:length(ds)
    d = ds(k);
    if isBad(d), continue; end

    Fd = F(d);

    if ~isfinite(Fd), continue; end

    if isnan(Fa_prev)
        Fa_prev = Fd;
        d_prev = d;
        continue;
    end

    if Fa_prev * Fd < 0
        a = d_prev; b = d;
        break;
    end

    Fa_prev = Fd;
    d_prev = d;
end

if isnan(a)
    error("Could not find a sign-change bracket in [0, lambda/2). Try different ZL or f.");
end

%% (1) Bisection Method 
iterB = 0;
d_old = a;

while iterB < maxIter
    iterB = iterB + 1;

    m = 0.5*(a + b);
    if isBad(m)
        % nudging midpoint slightly if it hits a tan singularity
        m = m + 1e-9;
    end

    Fm = F(m);

    errB = abs(m - d_old);   % here we change based error
    d_old = m;

    if abs(Fm) < tol || errB < tol
        break;
    end

    if F(a)*Fm < 0
        b = m;
    else
        a = m;
    end
end

d_bisect = m;
finalErrB = abs(F(d_bisect));

fprintf("Bisection Results\n");
fprintf("d = %.12e m (%.6f * lambda)\n", d_bisect, d_bisect/lambda);
fprintf("final error (|F(d)|) = %.3e\n", finalErrB);
fprintf("iterations = %d\n\n", iterB);

%% 2) Newton-Raphson Method 
% using midpoint of bracket as initial guess (reasonable and reproducible)
d = 0.5*(a + b);

iterN = 0;
while iterN < maxIter
    iterN = iterN + 1;

    if isBad(d)
        d = d + 1e-9;
    end

    Fd = F(d);

    % central difference derivative (still Newton update, just derivative approximation)
    h = 1e-6*lambda; % a small step relative to wavelength
    d1 = d - h; d2 = d + h;

    % ensuring to stay within domain and away from singularities
    d1 = max(d1, d_min); d2 = min(d2, d_max);
    if isBad(d1), d1 = d1 + 1e-9; end
    if isBad(d2), d2 = d2 - 1e-9; end

    F1 = F(d1);
    F2 = F(d2);

    dF = (F2 - F1) / (d2 - d1);

    if ~isfinite(dF) || dF == 0
        error("Newton failed: derivative became invalid/zero.");
    end

    d_new = d - Fd/dF;

    % Safeguard: if Newton jumps خارج المجال, damp it back
    if d_new <= d_min || d_new >= d_max || isBad(d_new) || ~isfinite(F(d_new))
        d_new = 0.5*(d + d_bisect); % damped step toward bisection solution
    end

    errN = abs(d_new - d);
    d = d_new;

    if abs(F(d)) < tol || errN < tol
        break;
    end
end

d_newton = d;
finalErrN = abs(F(d_newton));

fprintf("Newton-Raphson Results\n");
fprintf("d = %.12e m (%.6f * lambda)\n", d_newton, d_newton/lambda);
fprintf("final error (|F(d)|) = %.3e\n", finalErrN);
fprintf("iterations = %d\n\n", iterN);

%% Compare 
fprintf("Comparison\n");
fprintf("|d_bisect - d_newton| = %.3e m\n", abs(d_bisect - d_newton));

%% local function: normalized input admittance y_in(d)/Y0 ----------
function y = y_in_norm(d, Z0, ZL, beta)
    % Zin(d) for lossless line
    t = tan(beta*d);
    Zin = Z0 * (ZL + 1j*Z0*t) / (Z0 + 1j*ZL*t);

    % normalize admittance: y = (1/Zin) / (1/Z0) = Z0/Zin
    y = Z0 ./ Zin;
end

% dense distance vector for plotting
d_plot = linspace(d_min, d_max, 4000);

F_plot = zeros(size(d_plot));
g_plot = zeros(size(d_plot));

for k = 1:length(d_plot)
    if abs(cos(beta*d_plot(k))) < 1e-6
        F_plot(k) = NaN;
        g_plot(k) = NaN;
    else
        ytemp = y_in_norm(d_plot(k), Z0, ZL, beta);
        g_plot(k) = real(ytemp);
        F_plot(k) = g_plot(k) - 1;
    end
end

%%  Plot 1: Nonlinear function F(d) 
figure;
plot(d_plot/lambda, F_plot, 'b', 'LineWidth', 1.6); hold on;
yline(0, 'k--', 'LineWidth', 1.2);
xline(d_bisect/lambda, 'r--', 'LineWidth', 1.5);
grid on;

xlabel('d / \lambda');
ylabel('F(d) = Re\{y_{in}\} - 1');
title('Nonlinear Function for RF Stub Matching');
legend('F(d)', 'Zero line', 'Solution location', 'Location', 'best');

%% Plot 2: real part of normalized admittance 
figure;
plot(d_plot/lambda, g_plot, 'm', 'LineWidth', 1.6); hold on;
yline(1, 'k--', 'LineWidth', 1.2);
xline(d_bisect/lambda, 'r--', 'LineWidth', 1.5);
grid on;

xlabel('d / \lambda');
ylabel('Re\{y_{in}(d)\}');
title('Normalized Input Conductance Along the Line');
legend('Re\{y_{in}\}', 'Matched conductance = 1', 'Solution location', 'Location', 'best');

%% Plot 3: Newton-Raphson convergence 
% Re run Newton briefly to record iteration history
d_hist = [];
F_hist = [];

d_temp = 0.5*(a + b);

for i = 1:10
    d_hist(end+1) = d_temp;
    F_hist(end+1) = abs(F(d_temp));

    h = 1e-6*lambda;
    dF = (F(d_temp+h) - F(d_temp-h))/(2*h);
    d_temp = d_temp - F(d_temp)/dF;
end

figure;
semilogy(0:length(F_hist)-1, F_hist, 'o-', 'LineWidth', 1.6);
grid on;

xlabel('Iteration number');
ylabel('|F(d)| (log scale)');
title('Newton-Raphson Convergence');