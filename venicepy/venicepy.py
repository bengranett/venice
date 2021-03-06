# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.35
#
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

"""
A wrapper for Venice
"""

import _venicepy
import new
new_instancemethod = new.instancemethod
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'PySwigObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


class Mask(_object):
    """Proxy of C++ Mask class"""
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Mask, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Mask, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        """__init__(self, mask_file) -> Mask"""
        this = _venicepy.new_Mask(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _venicepy.delete_Mask
    __del__ = lambda self : None;
    def _check_point(*args):
        """_check_point(self, ra, dec, out)"""
        return _venicepy.Mask__check_point(*args)

    def check_point(self, ra, dec):
        """ ra,dec -> bool 
    The convention used in venice is False means INSIDE the mask, True OUTSIDE"""
        return self._check_point(ra,dec,len(ra)) > 0  # return a boolean type


    def _random_cat(*args):
        """_random_cat(self, bounds, inout, out_ra, out_dec) -> int"""
        return _venicepy.Mask__random_cat(*args)

    def random_cat(self, bounds, nrandom, inout='outside'):
        assert(len(bounds)==4)
        assert(bounds[0]<bounds[2])
        assert(bounds[1]<bounds[3])
        n = int(nrandom)

        print type(inout),type('ciao')
        if type(inout)==type('ciao'):
            print inout
            if inout[0]=='i':
                inout = 0
            elif inout[0]=='o':
                inout = 1
            else:
                print "Warning! random_cat: inout parameter should start with 'i' or 'o' or be integer 0 or 1. Using default outside."
                inout = 1
        
        inout = int(inout)
        assert(inout==0 or inout==1)

        n,ra,dec = self._random_cat(bounds,inout,n,n)
        return ra,dec

Mask_swigregister = _venicepy.Mask_swigregister
Mask_swigregister(Mask)



