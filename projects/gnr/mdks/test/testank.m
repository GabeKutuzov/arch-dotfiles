function mdks = testank(X1, X2, D)

% Test ank ribbon version of 2D MDKS
% V1A, 07/27/15, GHR
 
nx = 2;     % Number of data sets
tk = 2;     % Dimensions (must be 2)

% Two test data sets (1st dim: x, then y; 2nd dim: point index
nd1 = size(X1,2);
nd2 = size(X2,2);
if nd1 ~= nd2, error('Dimensions of X1 and X2 are not the same'); end
nd = nd1;
X = cell(2,1);
X{1} = X1;
X{2} = X2;

% Sort and make bricks
Divs = 2^D;
BrickCounts = zeros(Divs,Divs,nx);
BrickHeaders = zeros(Divs,Divs,nx);
BrickMembers = zeros(nd,nx);
MxDat = zeros(Divs,tk);
MnDat = zeros(Divs,tk);
WidDat = zeros(Divs,tk);
RibNo = zeros(nd,nx,tk);
X12 = [X1, X2];
nprib = floor(nx*nd/Divs);
bprem = nx*nd - nprib*Divs;
for ik = 1:tk
   [SX12,IS] = sort(X12(ik,:));
   mrib = nprib + (bprem > 0);
   mribbeg = 1;
   irib = 1;
   for jdat=1:size(SX12,2)
      idat = IS(jdat);
      if (idat > nd)
         jx = 2;
         idat = idat - nd;
      else
         jx = 1;
      end
      
      if jdat == mribbeg, MnDat(irib,ik) = SX12(jdat); end
      if jdat == mrib
         MxDat(irib,ik) = SX12(jdat);
         WidDat(irib,ik) = MxDat(irib,ik) - MnDat(irib,ik);
         end
      RibNo(idat,jx,ik) = irib;
      if (jdat == mrib)
         irib = irib + 1;
         bprem = bprem - 1;
         mrib = mrib + nprib + (bprem > 0);
         end
      end
   end

% Count data points in each brick
for jx=1:nx
   for idat=1:nd
      ixr = RibNo(idat,jx,1);
      iyr = RibNo(idat,jx,2);
      BrickCounts(ixr,iyr,jx) = BrickCounts(ixr,iyr,jx) + 1;
      BrickMembers(idat,jx) = BrickHeaders(ixr,iyr,jx);
      BrickHeaders(ixr,iyr,jx) = idat;
      end
   end

mdks = 0;
for idat=1:nd
   qtot = zeros(2^tk,nx);
   ixr = RibNo(idat,1,1);
   iyr = RibNo(idat,1,2);
   xybad = 0;
   xtest = X1(1,idat);
   ytest = X1(2,idat);
   if WidDat(ixr,1) == 0 || WidDat(iyr,1) == 0
      xybad = 1;
   else
      xright = (MxDat(ixr,1) - xtest)/WidDat(ixr,1);
      ybot = (MxDat(iyr,1) - ytest)/WidDat(iyr,1);
      end

   for jx=1:nx
      brflgs = zeros(tk,Divs,Divs);
      Xj = X{jx};
      for ibx=1:Divs
         if ibx < RibNo(idat,jx,1), xpos = -1;
         elseif ibx > RibNo(idat,jx,1), xpos = 1;
         else, xpos = 0;
         end
         for iby=1:Divs
            if iby < RibNo(idat,jx,1), ypos = -1;
            elseif iby > RibNo(idat,jx,1), ypos = 1;
            else, ypos = 0;
            end
            % Double overlap or bad ribbon
            if xybad || (xpos == 0 && ypos == 0)
               % Do old thing
               jdat = BrickHeaders(ibx,iby,jx);
               while jdat
                  iq = 1;
                  if (Xj(1,jdat) > xtest), iq = iq + 1; end
                  if (Xj(2,jdat) > ytest), iq = iq + 2; end
                  qtot(iq,jx) = qtot(iq,jx) + 1;
                  jdat = BrickMembers(jdat,jx);
                  end
            % Clean bricks
            elseif xpos < 0 && ypos < 0
               qtot(1,jx) = qtot(1,jx) + BrickCounts(ibx,iby,jx);
            elseif xpos > 0 && ypos < 0
               qtot(2,jx) = qtot(2,jx) + BrickCounts(ibx,iby,jx);
            elseif xpos < 0 && ypos > 0
               qtot(3,jx) = qtot(3,jx) + BrickCounts(ibx,iby,jx);
            elseif xpos > 0 && ypos > 0
               qtot(4,jx) = qtot(4,jx) + BrickCounts(ibx,iby,jx);
            % 1D overlaps
            elseif xpos == 0
               if brflgs(1,ibx,iby) == 0
                  if ypos < 0
                     npts = sum(BrickCounts(ibx,1:iby-1,jx));
                     nleft = (1.0 - xright)*npts;
                     qtot(1,jx) = qtot(1,jx) + nleft;
                     qtot(2,jx) = qtot(2,jx) + npts - nleft;
                  else
                     npts = sum(BrickCounts(ibx,iby+1:end,jx));
                     nleft = (1.0 - xright)*npts;
                     qtot(3,jx) = qtot(3,jx) + nleft;
                     qtot(4,jx) = qtot(4,jx) + npts - nleft;
                     end
                  brflgs(1,ibx,iby) = 1;
                  end
            else  % ypos == 0 case
               if brflgs(2,ibx,iby) == 0
                  if xpos < 0
                     npts = sum(BrickCounts(1:ibx-1,iby,jx));
                     nup = (1.0 - ybot)*npts;
                     qtot(1,jx) = qtot(1,jx) + nup;
                     qtot(3,jx) = qtot(3,jx) + npts - nup;
                  else
                     npts = sum(BrickCounts(ibx+1:end,iby,jx));
                     nup = (1.0 - ybot)*npts;
                     qtot(2,jx) = qtot(2,jx) + nup;
                     qtot(4,jx) = qtot(4,jx) + npts - nup;
                     end
                  brflgs(2,ibx,iby) = 1;
                  end
               end % End effective case switch
            end   % End iby loop
         end   % End ibx loop
      end % End jx loop

   % Loop over quads, find largest difference
   qdiff = abs(ceil(qtot(1,1)) - ceil(qtot(1,2)));
   if (qdiff > mdks), mdks = qdiff; end
   qdiff = abs(floor(qtot(2,1)) - floor(qtot(2,2)));
   if (qdiff > mdks), mdks = qdiff; end
   qdiff = abs(ceil(qtot(3,1)) - ceil(qtot(3,2)));
   if (qdiff > mdks), mdks = qdiff; end
   qdiff = abs(floor(qtot(4,1)) - floor(qtot(4,2)));
   if (qdiff > mdks), mdks = qdiff; end
   end % Loop over test points
mdks = mdks/sqrt(nd1);


            



            
             
      
