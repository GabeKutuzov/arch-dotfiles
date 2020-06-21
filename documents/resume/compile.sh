#!/bin/bash

groff -ms resume.ms -T pdf > resume.pdf &&
   mupdf resume.pdf
