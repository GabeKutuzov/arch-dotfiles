% (c) Copyright 2011-2012, The Rockefeller University *11306*
function [citimg, box_coord, obj_contour] = ...
   citrdimg(fname, mxxy, kopt, afname)
% $Id: citrdimg.m 9 2012-11-21 22:00:13Z  $
% Function to read an image from the CIT 101 image database
% and possibly crop it or retrieve data to mask out background.
%
% This function is explicitly designed to be called from citmexget
% (a mex file) in order to hide the details of reading a jpeg file
% from the C code.  Therefore, argument type checking is minimal.
% Subscripts into mxxy and kopt arrays are 1-based equivalents of
% those defined in citmexget.c.
%
% Arguments:
%  fname       Path to the image file to be read.
%  mxxy        A 1x4 double array containing (1) maximum image size
%              along x (horiz), (2) cropping border extension along
%              x, (3) maximum image size along y (vert), and (4)
%              cropping border extension along y.  If the image is
%              smaller than these values in the respective dimension,
%              the entire row or column is returned.  If larger, a
%              subimage centered on the file image is returned.
%  kopt        An array of 3 Matlab logical values as follows:
%              (1) If 'true', colored images are returned as such
%              (with separate red-green-blue values).  If 'false',
%              colored images are returned as gray-level equivalents.
%              (2) Images are cropped to the dimensions given in the
%              annotation file.  Argument 'afname' is required.
%              (3) The caller wishes to set densities outside the
%              figure outline given in the annotation file to 0.
%              Argument 'afname' is required and arguments box_coord
%              and obj_contour are returned.
%  afname      Path to the annotation file for this image as supplied
%              in the CIT 101 database (required for cropping/masking).
%
% Return values:
%  citimg      A uint8 Matlab array of size m x n, where m is no
%              larger than mxxy(3) and n is no larger than mxxy(1),
%              i.e. imread returns images with cols in dim. 1 and
%              rows in dim. 2, the transpose of the way we normally
%              think of an image.  If the image is colored, and
%              kopt(1) is 'true', then the image size is m x n x 3.
%  box_coord   These are now the sizes of any borders along low y,
%              high y, low x, high x, bzw, between the edges of the
%              image returned and the coordinate origins used for
%              object contours, i.e. box_coord(1) and box_coord(3)
%              are the corrections to be added to the y and x
%              object contours when painting out background.  Note
%              that if mxxy limits are small, these can be negative.
%              Returned only if kopt(3) is set.
%  obj_contour Contour information from annotation file, returned
%              only if kopt(3) is set.
%
%  V1A, 11/23/11, GNR - New program
%  V2A, 12/19/11, GNR - Add cropping and masking options
%  ==>, 12/22/11, GNR - Last mod before committing to svn repository
%  V3A, 11/08/12, GNR - Add cropping border and deferred painting,
%                       correct return/destroy annotation logic.

% Check that optional afname argument is present when needed
if kopt(2) | kopt(3)
   if nargin ~= 4
      error('Error 653: annotation file name missing');
   else
      load(afname, 'box_coord', 'obj_contour');
      end
   end

% Read image info and image
iinfo = imfinfo(fname);
TColor = strcmp(iinfo.ColorType, 'truecolor');
citimg = imread(fname, 'jpg');

% Truncate the image if necessary--
% The 'PixelRegion' parameter in the Matlab doc apparently
% only applies to JPEG 2000, whatever that is.
if kopt(2)
   y1 = box_coord(1) - mxxy(4);
   if y1 < 1, y1 = 1; end
   y2 = box_coord(2) + mxxy(4);
   if y2 > iinfo.Height, y2 = iinfo.Height; end
   x1 = box_coord(3) - mxxy(2);
   if x1 < 1, x1 = 1; end
   x2 = box_coord(4) + mxxy(2);
   if x2 > iinfo.Width, x2 = iinfo.Width; end
else
   y1 = 1;
   y2 = iinfo.Height;
   x1 = 1;
   x2 = iinfo.Width;
   end

twd = x2 - x1 + 1;
if twd > mxxy(1)
   xadj = floor((twd - mxxy(1))/2);
   x1 = x1 + xadj;
   x2 = x1 + mxxy(1) - 1;
else
   xadj = 0;
   end

tht = y2 - y1 + 1;
if tht > mxxy(3)
   yadj = floor((tht - mxxy(3))/2);
   y1 = y1 + yadj;
   y2 = y1 + mxxy(3) - 1;
else
   yadj = 0;
   end

if y1 ~= 1 || y2 ~= iinfo.Height || x1 ~= 1 || x2 ~= iinfo.Width
   if TColor
      citimg = citimg(y1:y2,x1:x2,:);
   else
      citimg = citimg(y1:y2,x1:x2);
      end
   end

% See if it is necessary to reduce color to grayscale
if TColor && ~kopt(1)
   citimg = uint8(mean(citimg,3));
   end

% Destroy box_coord and obj_contour if not needed by caller
if kopt(3)
   box_coord(1) = box_coord(1) - y1;
   box_coord(2) = box_coord(2) - y1;
   box_coord(3) = box_coord(3) - x1;
   box_coord(4) = box_coord(4) - x1;
elseif kopt(2)
   clear('box_coord', 'obj_contour');
   end

