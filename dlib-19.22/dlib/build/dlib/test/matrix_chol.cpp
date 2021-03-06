// Copyright (C) 2009  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.


#include <dlib/matrix.h>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include "../stl_checked.h"
#include "../array.h"
#include "../rand.h"
#include <dlib/string.h>

#include "tester.h"

namespace  
{

    using namespace test;
    using namespace dlib;
    using namespace std;

    logger dlog("test.matrix_chol");

    dlib::rand rnd;

// ----------------------------------------------------------------------------------------

    template <typename mat_type>
    const matrix<typename mat_type::type> symm(const mat_type& m) { return m*trans(m); }

// ----------------------------------------------------------------------------------------

    template <typename type>
    const matrix<type> randmat(long r, long c)
    {
        matrix<type> m(r,c);
        for (long row = 0; row < m.nr(); ++row)
        {
            for (long col = 0; col < m.nc(); ++col)
            {
                m(row,col) = static_cast<type>(rnd.get_random_double()); 
            }
        }

        return m;
    }

    template <typename type, long NR, long NC>
    const matrix<type,NR,NC> randmat()
    {
        matrix<type,NR,NC> m;
        for (long row = 0; row < m.nr(); ++row)
        {
            for (long col = 0; col < m.nc(); ++col)
            {
                m(row,col) = static_cast<type>(rnd.get_random_double()); 
            }
        }

        return m;
    }

// ----------------------------------------------------------------------------------------

    template <typename matrix_type>
    void test_cholesky ( const matrix_type& m)
    {
        typedef typename matrix_type::type type;
        const type eps = 10*max(abs(m))*sqrt(std::numeric_limits<type>::epsilon());
        dlog << LDEBUG << "test_cholesky():  " << m.nr() << " x " << m.nc() << "  eps: " << eps;
        print_spinner();


        cholesky_decomposition<matrix_type> test(m);

        // none of the matrices we should be passing in to test_cholesky() should be non-spd.  
        DLIB_TEST(test.is_spd() == true);

        type temp;
        DLIB_TEST_MSG( (temp= max(abs(test.get_l()*trans(test.get_l()) - m))) < eps,temp);

        {
            matrix<type> mat = chol(m);
            DLIB_TEST_MSG( (temp= max(abs(mat*trans(mat) - m))) < eps,temp);
        }


        matrix<type> m2;
        matrix<type,0,1> col;

        m2 = identity_matrix<type>(m.nr());
        DLIB_TEST_MSG(equal(m*test.solve(m2), m2,eps),max(abs(m*test.solve(m2)- m2)));
        m2 = randmat<type>(m.nr(),5);
        DLIB_TEST_MSG(equal(m*test.solve(m2), m2,eps),max(abs(m*test.solve(m2)- m2)));
        m2 = randmat<type>(m.nr(),1);
        DLIB_TEST_MSG(equal(m*test.solve(m2), m2,eps),max(abs(m*test.solve(m2)- m2)));
        col = randmat<type>(m.nr(),1);
        DLIB_TEST_MSG(equal(m*test.solve(col), col,eps),max(abs(m*test.solve(m2)- m2)));

        // now make us a non-spd matrix
        if (m.nr() > 2)
        {
            matrix<type> sm(lowerm(m));
            sm(1,1) = 0;

            cholesky_decomposition<matrix_type> test2(sm);
            DLIB_TEST_MSG(test2.is_spd() == false,  test2.get_l());


            cholesky_decomposition<matrix_type> test3(sm*trans(sm));
            DLIB_TEST_MSG(test3.is_spd() == false,  test3.get_l());

            sm = sm*trans(sm);
            sm(1,1) = 5;
            sm(1,0) -= 1;
            cholesky_decomposition<matrix_type> test4(sm);
            DLIB_TEST_MSG(test4.is_spd() == false,  test4.get_l());
        }

    }

// ----------------------------------------------------------------------------------------

    void matrix_test_double()
    {

        test_cholesky(uniform_matrix<double>(1,1,1) + 10*symm(randmat<double>(1,1)));
        test_cholesky(uniform_matrix<double>(2,2,1) + 10*symm(randmat<double>(2,2)));
        test_cholesky(uniform_matrix<double>(3,3,1) + 10*symm(randmat<double>(3,3)));
        test_cholesky(uniform_matrix<double>(4,4,1) + 10*symm(randmat<double>(4,4)));
        test_cholesky(uniform_matrix<double>(15,15,1) + 10*symm(randmat<double>(15,15)));
        test_cholesky(uniform_matrix<double>(101,101,1) + 10*symm(randmat<double>(101,101)));

        typedef matrix<double,0,0,default_memory_manager, column_major_layout> mat;
        test_cholesky(mat(uniform_matrix<double>(101,101,1) + 10*symm(randmat<double>(101,101))));
    }

// ----------------------------------------------------------------------------------------

    void matrix_test_float()
    {

        test_cholesky(uniform_matrix<float>(1,1,1) + 2*symm(randmat<float>(1,1)));
        test_cholesky(uniform_matrix<float>(2,2,1) + 2*symm(randmat<float>(2,2)));
        test_cholesky(uniform_matrix<float>(3,3,1) + 2*symm(randmat<float>(3,3)));

        typedef matrix<float,0,0,default_memory_manager, column_major_layout> mat;
        test_cholesky(mat(uniform_matrix<float>(3,3,1) + 2*symm(randmat<float>(3,3))));
    }

// ----------------------------------------------------------------------------------------

    class matrix_tester : public tester
    {
    public:
        matrix_tester (
        ) :
            tester ("test_matrix_chol",
                    "Runs tests on the matrix cholesky component.")
        {
            //rnd.set_seed(cast_to_string(time(0)));
        }

        void perform_test (
        )
        {
            dlog << LINFO << "seed string: " << rnd.get_seed();

            dlog << LINFO << "begin testing with double";
            matrix_test_double();
            dlog << LINFO << "begin testing with float";
            matrix_test_float();
        }
    } a;

}



