% This is for comparison with Two Puncture.
twopc % runs script twopc.m

hsq = h^2;

%Dirchlet boundary conditions and initial guess.
U= zeros(n+1,n+1,n+1);
U(1,:,:) = T(1,:,:);
U(n+1,:,:) = T(n+1,:,:);
U(:,1,:) = T(:,1,:);
U(:,n+1,:) = T(:,n+1,:);
U(:,:,1) = T(:,:,1);
U(:,:,n+1) = T(:,:,n+1); % n-1 interior points. Index 2:n are interior points.

assert(length(X) == length(U))
assert(length(X) == length(T))
assert(length(X) == length(Asq))

R = -0.125*(Asq).*((C+U).^(-7)); % Right hand side term.
res = max(max(max(abs(T-U))));

iteration = zeros(1,it_jacobi+it_linear+1);
residual = zeros(1,it_jacobi+it_linear+1);
residual (1) = res;

onebysix = 1/6;
% Jacobi Iteration on nonlinear PDE.
disp('JACOBI ITERATION.')
count = 1;
while count <= it_jacobi
    U(2:n,2:n,2:n) = U(1:n-1,2:n,2:n)+U(3:n+1,2:n,2:n)+U(2:n,1:n-1,2:n)+U(2:n,3:n+1,2:n);
    U(2:n,2:n,2:n) = +U(2:n,2:n,1:n-1)+U(2:n,2:n,3:n+1)-hsq*R(2:n,2:n,2:n);
    U(2:n,2:n,2:n) = onebysix;
    res = max(max(max(abs(T-U))));
    residual (count+1) = res;
    iteration (count+1) = count;
    R = -0.125*(Asq).*((C+U).^(-7)); % update RHS
    count = count+1;
end
semilogy(iteration(1:it_jacobi+1),residual(1:it_jacobi+1),'--k','LineWidth',1.2,'DisplayName','Jacobi')
hold on

U0 = U;
disp('Starting with Linearization')
count=1;
while count <= it_linear
    disp(['Linearization Step ' num2str(count)])
    delU = zeros(n+1,n+1,n+1); % guess
    laplaceU0 = U0;
    laplaceU0(2:n,2:n,2:n) = (1/hsq)*(U0(1:n-1,2:n,2:n)+U0(3:n+1,2:n,2:n)+U0(2:n,1:n-1,2:n)+U0(2:n,3:n+1,2:n)+U0(2:n,2:n,1:n-1)+U0(2:n,2:n,3:n+1)-6*U0(2:n,2:n,2:n));
    Rv = -0.125*(Asq).*((C+U0).^(-7));
    Tv = T - U0;
    cons1 = (C+U0).^(-7);
    cons2 = (C+U0).^(-8);
    m = it_jacobi + count;
    B = zeros(n-1,n-1,n-1);
    Diff = zeros(n-1,n-1,n-1);

    B = onebysix*(delU(1:n-1,2:n,2:n)+delU(3:n+1,2:n,2:n)+delU(2:n,1:n-1,2:n)+delU(2:n,3:n+1,2:n)+delU(2:n,2:n,1:n-1)+delU(2:n,2:n,3:n+1)-hsq*Rv(2:n,2:n,2:n));
    delU(2:n,2:n,2:n) = OneMinusOmega*delU(2:n,2:n,2:n) + Omega*B;
    % Compute Residual
    Diff = abs(Tv(2:n,2:n,2:n)-delU(2:n,2:n,2:n));
    res = max(max(max(Diff)));
    residual (m+k) = res;
    iteration (m+k) = m+k; % Iteration Count
    Rv = -0.125*Asq.*(cons1 -7*delU.*cons2) - laplaceU0;
    
    
    %Plotting
    semilogy(iteration(m:m+k-1),residual(m:m+k-1),'-','Linewidth',1.2,'DisplayName',['SRJ ' num2str(count)])
    
    U0 = U0 + delU;
    count = count+1;
end

Out = C+U0;

legend('show')
xlabel('Iteration Count','Fontsize',15)
ylabel('|| r ||_\infty','Fontsize',15)
title('Linearized SRJ','Fontsize',15)
xlim([0,length(iteration)]);
grid on