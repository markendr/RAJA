.. ##
.. ## Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
.. ## and RAJA project contributors. See the RAJA/LICENSE file
.. ## for details.
.. ##
.. ## SPDX-License-Identifier: (BSD-3-Clause)
.. ##

.. _dotproduct-label:

-----------------------------------
Vector Dot Product (Sum Reduction)
-----------------------------------

Key RAJA features shown in this example:

  * ``RAJA::forall`` loop execution template
  * ``RAJA::RangeSegment`` iteration space construct
  * RAJA execution policies
  * ``RAJA::ReduceSum`` sum reduction template
  * RAJA reduction policies


In the example, we compute a vector dot product, 'dot = (a,b)', where 
'a' and 'b' are two vectors length N and 'dot' is a scalar. Typical
C-style code to compute the dot product and print its value afterward is: 

.. literalinclude:: ../../../../examples/tut_dot-product.cpp
   :start-after: _csytle_dotprod_start
   :end-before: _csytle_dotprod_end
   :language: C++

Note that this operation performs a *reduction*, a computational pattern that
produces a single result from a set of values. Reductions present a variety
of issues that must be addressed to operate properly in parallel. 

^^^^^^^^^^^^^^^^^^^^^
RAJA Variants
^^^^^^^^^^^^^^^^^^^^^

Different programming models support parallel reduction operations differently.
Some models, such as CUDA, do not provide support for reductions at all and 
so such operations must be explicitly coded by users. It can be challenging 
to generate a correct and high performance implementation. RAJA provides 
portable reduction types that make it easy to perform reduction operations
in loop kernels. The RAJA variants of the dot product computation show how 
to use the ``RAJA::ReduceSum`` sum reduction template type. RAJA provides
other reduction types and also allows multiple reduction operations to be
performed in a single kernel along with other computation. Please see 
:ref:`reductions-label` for an example that does this.

Each RAJA reduction type takes a `reduce policy` template argument, which
**must be compatible with the execution policy** applied to the kernel
in which the reduction is used. Here is the RAJA sequential variant of the dot 
product computation:

.. literalinclude:: ../../../../examples/tut_dot-product.cpp
   :start-after: _rajaseq_atomic_histogram_start
   :end-before: _rajaseq_atomic_histogram_end
   :language: C++

The sum reduction object is defined by specifying the reduction 
policy ``RAJA::seq_reduce``, which matches the loop execution policy, and
a reduction value type (i.e., 'double'). An initial value of zero for the 
sum is passed to the reduction object constructor. After the kernel executes,
we use the 'get' method to retrieve the reduced value.

The OpenMP multithreaded variant of the loop is implemented similarly:

.. literalinclude:: ../../../../examples/tut_dot-product.cpp
   :start-after: _rajaomp_dotprod_start
   :end-before: _rajaomp_dotprod_end
   :language: C++

Here, we use the ``RAJA::omp_reduce`` reduce policy to match the OpenMP
loop execution policy.

The RAJA CUDA variant is achieved by using appropriate loop execution and 
reduction policies:

.. literalinclude:: ../../../../examples/tut_dot-product.cpp
   :start-after: _rajacuda_dotprod_start
   :end-before: _rajacuda_dotprod_end
   :language: C++

Here, the CUDA reduce policy ``RAJA::cuda_reduce`` matches the CUDA 
loop execution policy. Note that the CUDA thread block size is not 
specified in the reduce policy as it will use the same value as the
loop execution policy.

Similarly, for the RAJA HIP variant:

.. literalinclude:: ../../../../examples/tut_dot-product.cpp
   :start-after: _rajahip_dotprod_start
   :end-before: _rajahip_dotprod_end
   :language: C++

It is worth noting how similar the code looks for each of these variants.
The loop body is identical for each and only the loop execution policy
and reduce policy types change.

The file ``RAJA/examples/tut_dot-product.cpp`` contains the complete 
working example code.
