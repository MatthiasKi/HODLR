#include "Matrix_Factorizer.hpp"
 
void Matrix_Factorizer::maxAbsVector(const Vec& v, 
                                     const std::set<int>& allowed_indices,
                                     dtype& max, int& index
                                    ) 
{
    std::set<int>::iterator it;
    index = *allowed_indices.begin();
    max   = v(index);

    for(it = allowed_indices.begin(); it != allowed_indices.end(); it++) 
    {
        if(fabs(v(*it))>fabs(max)) 
        {
            index   =   *it;
            max     =   v(index);
        }
    }
}

void Matrix_Factorizer::rookPiv(Mat& L, Mat& R, double tolerance,
                                int n_row_start, int n_col_start,
                                int n_rows, int n_cols
                               )
{
    // Indices which have been used:
    std::vector<int> row_ind;      
    std::vector<int> col_ind;

    // Indices that are remaining:
    std::set<int> remaining_row_ind;
    std::set<int> remaining_col_ind;
    
    // Bases:
    std::vector<Vec> u; 
    std::vector<Vec> v;

    for(int k = 0; k < n_rows; k++) 
    {
        remaining_row_ind.insert(k);
    }
    
    for(int k = 0; k < n_cols; k++) 
    {
        remaining_col_ind.insert(k);
    }

    dtype max, gamma;

    // Initialize the matrix norm and the the first row index
    dtype_base matrix_norm = 0;
    row_ind.push_back(0);
    remaining_row_ind.erase(0);

    // Stores the pivot entry of the considered row / col:
    int pivot;

    // This would get updated:
    int computed_rank = 0;
    Vec row, col;
    // These quantities in finding the stopping criteria:
    dtype_base row_squared_norm, row_norm, col_squared_norm, col_norm;

    // So these would be particularly useful for poorly conditioned matrices:
    int max_tries = 10;
    int count;

    // Repeat till the desired tolerance is obtained
    do 
    {
        // Generation of the row
        // Row of the residuum and the pivot column
        // By calling row_ind.back(), we are getting the last pushed number
        row = this->A->getRow(n_row_start + row_ind.back(), n_col_start, n_cols);
        
        for(int i = 0; i < computed_rank; i++) 
        {
            row = row - u[i](row_ind.back()) * v[i];
        }

        this->maxAbsVector(row, remaining_col_ind, max, pivot);
        count = 0;

        // Alternating upon each call:
        bool eval_at_end = false;
        // Toggling randomness
        bool use_randomization = true;

        // This while loop is needed if in the middle of the algorithm the 
        // row happens to be exactly the linear combination of the previous rows 
        // upto some tolerance. i.e. prevents from ACA throwing false positives
        while (fabs(max) < tolerance && 
               count < max_tries   && 
               remaining_col_ind.size() > 0 && 
               remaining_row_ind.size() > 0
              ) 
        {
            row_ind.pop_back();
            int new_row_ind;

            // When rank < 3, we will just choose entries from the ends of the matrix:
            if(computed_rank < 3)
            {
                if(eval_at_end == true)
                {
                    new_row_ind = *remaining_row_ind.end();
                }

                else
                {
                    new_row_ind = *remaining_row_ind.begin();
                }
    
                eval_at_end = !(eval_at_end);
            }

            // However, when we have rank >=3, we will choose the entries such that
            // the newly picked entry is at the mid-point of the already chosen ones:
            else
            {
                if(use_randomization == true)
                {
                    std::set<int>::const_iterator it(remaining_row_ind.begin());
                    std::advance(it, rand() % remaining_row_ind.size());
                    new_row_ind = *it;
                }

                else
                {
                    std::vector<int> row_ind_sort(row_ind);
                    std::sort(row_ind_sort.begin(), row_ind_sort.end());
                    std::vector<int> row_ind_diff(row_ind_sort.size() - 1);

                    int max = 0;
                    int idx = 0;

                    for(int i = 0; i < row_ind_sort.size() - 1; i++)
                    {
                        row_ind_diff[i] = row_ind_sort[i+1] - row_ind_sort[i];
                        if(row_ind_diff[i] > max)
                        {
                            idx = i;
                            max = row_ind_diff[i];
                        }
                    }

                    new_row_ind = row_ind_sort[idx] + max / 2;
                }

                use_randomization = !(use_randomization);
            }

            row_ind.push_back(new_row_ind);
            remaining_row_ind.erase(new_row_ind);
            // Generation of the row
            // Row of the residuum and the pivot column
            row = this->A->getRow(n_row_start + new_row_ind, n_col_start, n_cols);
            for(int i = 0; i < computed_rank; i++) 
            {
                row = row - u[i](row_ind.back()) * v[i];
            }

            this->maxAbsVector(row, remaining_col_ind, max, pivot);
            count++;
        }

        // In case it failed to resolve in the previous step, 
        // we break out of the dowhile loop:
        if (count == max_tries || 
            remaining_col_ind.size() == 0 || 
            remaining_row_ind.size() == 0
           )
        {
            break;
        } 

        // Resetting count back to zero for columns:
        count = 0;
        
        col_ind.push_back(pivot);
        remaining_col_ind.erase(pivot);
        // Normalizing constant
        gamma = dtype_base(1.0) / max;

        // Generation of the column
        // Column of the residuum and the pivot row
        col = this->A->getCol(n_col_start + col_ind.back(), n_row_start, n_rows);
        for(int i = 0; i < computed_rank; i++) 
        {
            col = col - v[i](col_ind.back()) * u[i];
        }

        this->maxAbsVector(col, remaining_row_ind, max, pivot);
        // Repeating the same randomization we carried out for the rows, now for the columns:
        while (fabs(max)<tolerance && 
               count < max_tries && 
               remaining_col_ind.size() >0 && 
               remaining_row_ind.size() >0
              ) 
        {
            col_ind.pop_back();

            int new_col_ind;

            if(col_ind.size() < 3)
            {
                if(eval_at_end)
                {
                    new_col_ind = *remaining_col_ind.end();
                }

                else
                {
                    new_col_ind = *remaining_col_ind.begin();
                }
    
                eval_at_end = !eval_at_end;
            }

            else
            {                
                if(use_randomization == true)
                {
                    std::set<int>::const_iterator it(remaining_col_ind.begin());
                    std::advance(it, rand() % remaining_col_ind.size());
                    new_col_ind = *it;
                }

                else
                {
                    std::vector<int> col_ind_sort(col_ind);
                    std::sort(col_ind_sort.begin(), col_ind_sort.end());
                    std::vector<int> col_ind_diff(col_ind_sort.size() - 1);

                    int max = 0;
                    int idx = 0;

                    for(int i = 0; i < col_ind_sort.size() - 1; i++)
                    {
                        col_ind_diff[i] = col_ind_sort[i+1] - col_ind_sort[i];
                        if(col_ind_diff[i] > max)
                        {
                            idx = i;
                            max = col_ind_diff[i];
                        }
                    }

                    new_col_ind = col_ind_sort[idx] + max / 2;
                }

                use_randomization = !(use_randomization);
            }

            col_ind.push_back(new_col_ind);
            remaining_col_ind.erase(new_col_ind);

            // Generation of the column
            // Column of the residuum and the pivot row:
            col = this->A->getCol(n_col_start + new_col_ind, n_row_start, n_rows);
            for(int i = 0; i < computed_rank; i++) 
            {
                col = col - v[i](col_ind.back()) * u[i];
            }

            this->maxAbsVector(col, remaining_row_ind, max, pivot);
            count++;
        }

        row_ind.push_back(pivot);
        remaining_row_ind.erase(pivot);

        // New vectors
        u.push_back(gamma * col);
        v.push_back(row);

        // New approximation of matrix norm
        row_squared_norm = row.squaredNorm();
        row_norm         = sqrt(row_squared_norm);

        col_squared_norm = col.squaredNorm();
        col_norm         = sqrt(col_squared_norm);

        // Updating the matrix norm:
        matrix_norm += std::abs(gamma * gamma * row_squared_norm * col_squared_norm);

        for(int j = 0; j < computed_rank; j++) 
        {
            matrix_norm += 2.0 * std::abs(u[j].dot(u.back())) 
                               * std::abs(v[j].dot(v.back()));
        }
        
        computed_rank++;
    } 
    while(computed_rank * (n_rows + n_cols) * row_norm * col_norm > 
          fabs(max) * tolerance * matrix_norm && 
          computed_rank < fmin(n_rows, n_cols)
         );

    // If the computed_rank is >= to full-rank
    // then return the trivial full-rank decomposition
    if (computed_rank >= fmin(n_rows, n_cols) - 1) 
    {
        if (n_rows < n_cols) 
        {
            L = Mat::Identity(n_rows, n_rows);
            R = this->A->getMatrix(n_row_start, n_col_start, n_rows, n_cols).transpose();
            computed_rank = n_rows;
        }

        else 
        {
            L = this->A->getMatrix(n_row_start, n_col_start, n_rows, n_cols);
            R = Mat::Identity(n_cols, n_cols);
            computed_rank = n_cols;
        }
    }
    
    // This is when ACA has succeeded:
    else 
    {
        L = Mat(n_rows, computed_rank);
        R = Mat(n_cols, computed_rank);
        
        for (int j = 0; j < computed_rank; j++) 
        {
            L.col(j) = u[j];
            R.col(j) = v[j];
        }
    }
}

void Matrix_Factorizer::queenPiv(Mat& L, Mat& R, double tolerance,
                                 int n_row_start, int n_col_start,
                                 int n_rows, int n_cols
                                )
{
    // Indices which have been used:
    std::vector<int> row_ind;      
    std::vector<int> col_ind;

    // Indices that are remaining:
    std::set<int> remaining_row_ind;
    std::set<int> remaining_col_ind;
    
    // Bases:
    std::vector<Vec> u; 
    std::vector<Vec> v;

    for(int k = 0; k < n_rows; k++) 
    {
        remaining_row_ind.insert(k);
    }
    
    for(int k = 0; k < n_cols; k++) 
    {
        remaining_col_ind.insert(k);
    }

    dtype max1, max2, max, gamma;

    // Initialize the matrix norm:
    dtype_base matrix_norm = 0;
    // Stores the pivot entry of the considered row / col:
    int pivot1, pivot2, pivot;

    // This would get updated:
    int computed_rank = 0;
    Vec diag1, diag2, row, col;
    // These quantities in finding the stopping criteria:
    dtype_base row_squared_norm, row_norm, col_squared_norm, col_norm;

    // Repeat till the desired tolerance is obtained
    do 
    {
        std::cout << "Iteration Number:" << computed_rank << std::endl << std::endl;
        if(computed_rank == 0)
        {
            diag1 = this->A->getDiag1(n_row_start, n_col_start, n_rows, n_cols);
        }

        else
        {
            diag1 = this->A->getDiag1(n_row_start + row_ind.back(), n_col_start + col_ind.back(), n_rows, n_cols);
        }

        Mat res = this->A->getMatrix(0, 0, 5, 5);
        for(int i = 0; i < computed_rank; i++)
        {
            res = res - (u[i] * v[i].transpose());
        }

        std::cout << "Matrix Now is:" << std::endl;
        std::cout <<  res << std::endl << std::endl;
        
        for(int i = 0; i < computed_rank; i++)
        {
            int row_idx, col_idx;
            #pragma omp parallel for 
            for (int j = 0; j < diag1.size(); j++)
            {
                if(n_cols > n_rows)
                {
                    row_idx = this->A->mod((n_row_start + row_ind.back()) - (n_col_start + col_ind.back()) + j, n_rows);
                    col_idx = j;
                }

                else
                {
                    row_idx = j;
                    col_idx = this->A->mod((n_col_start + col_ind.back()) - (n_row_start + row_ind.back()) + j, n_cols);
                }

                diag1(j) = diag1(j) - u[i](row_idx) * v[i](col_idx);
            }
        }

        std::cout << "Diagonal 1:" << std::endl << diag1 << std::endl << std::endl;

        if(computed_rank == 0)
            diag2 = this->A->getDiag2(n_row_start, n_col_start, n_rows, n_cols);
        else
            diag2 = this->A->getDiag2(n_row_start + row_ind.back(), n_col_start + col_ind.back(), n_rows, n_cols);

        for(int i = 0; i < computed_rank; i++)
        {
            int row_idx, col_idx;
            #pragma omp parallel for 
            for (int j = 0; j < diag2.size(); j++)
            {
                if(n_cols > n_rows)
                {
                    row_idx = this->A->mod((n_row_start + row_ind.back()) + (n_col_start + col_ind.back()) - j, n_rows);
                    col_idx = j;
                }

                else
                {
                    row_idx = j;
                    col_idx = this->A->mod((n_col_start + col_ind.back()) + (n_row_start + row_ind.back()) - j, n_cols);
                }

                diag2(j) = diag2(j) - u[i](row_idx) * v[i](col_idx);
            }
        }

        std::cout << "Diagonal 2:" << std::endl << diag2 << std::endl << std::endl;
        if(n_cols > n_rows)
            this->maxAbsVector(diag1, remaining_col_ind, max1, pivot1);
        else
            this->maxAbsVector(diag1, remaining_row_ind, max1, pivot1);

        std::cout << "MAX1:" << max1 << std::endl;
        std::cout << "PIV1:" << pivot1 << std::endl;

        if(n_cols > n_rows)
            this->maxAbsVector(diag2, remaining_col_ind, max2, pivot2);
        else
            this->maxAbsVector(diag2, remaining_row_ind, max2, pivot2);

        std::cout << "MAX2:" << max2 << std::endl;
        std::cout << "PIV2:" << pivot2 << std::endl;

        int new_row_ind, new_col_ind;
        if(fabs(max2) > fabs(max1))
        {
            std::cout << "We are choosing diag2" << std::endl;
            max   = max2;
            pivot = pivot2;

            if(n_cols > n_rows)
            {
                new_row_ind = this->A->mod((n_col_start + col_ind.back()) + (n_row_start + row_ind.back()) - pivot, n_rows);
                new_col_ind = pivot;
            }

            else
            {
                new_row_ind = pivot;
                new_col_ind = this->A->mod((n_col_start + col_ind.back()) + (n_row_start + row_ind.back()) - pivot, n_cols);
            }
        }

        else
        {
            std::cout << "We are choosing diag1" << std::endl;
            max   = max1;
            pivot = pivot1;

            if(n_cols > n_rows)
            {
                if(computed_rank == 0)
                    new_row_ind = this->A->mod(n_col_start - n_row_start + pivot, n_rows);

                else
                    new_row_ind = this->A->mod((n_col_start + col_ind.back()) - (n_row_start + row_ind.back()) + pivot, n_rows);

                new_col_ind = pivot;
            }

            else
            {
                new_row_ind = pivot;

                if(computed_rank == 0)
                    new_col_ind = this->A->mod(n_col_start - n_row_start + pivot, n_cols);
                else
                    new_col_ind = this->A->mod((n_col_start + col_ind.back()) - (n_row_start + row_ind.back()) + pivot, n_cols);
            }
        }

        // If the max value ~ machine ε, we terminate the process:
        if(fabs(max) < 1e-14)
            break;

        std::cout << "ROWI:" << new_row_ind << std::endl;
        std::cout << "COLI:" << new_col_ind << std::endl << std::endl;

        col_ind.push_back(new_col_ind);
        remaining_col_ind.erase(new_col_ind);
        // Normalizing constant
        gamma = dtype_base(1.0) / max;
        row_ind.push_back(new_row_ind);
        remaining_row_ind.erase(new_row_ind);

        // New vectors
        row = this->A->getRow(new_row_ind, n_col_start, n_cols);
        for(int i = 0; i < computed_rank; i++) 
        {
            row = row - u[i](row_ind.back()) * v[i];
        }

        col = this->A->getCol(new_col_ind, n_row_start, n_rows);
        for(int i = 0; i < computed_rank; i++) 
        {
            col = col - v[i](col_ind.back()) * u[i];
        }

        u.push_back(gamma * col);
        v.push_back(row);

        std::cout << u.back() << std::endl;
        std::cout << v.back() << std::endl;
        std::cout << u.back() * v.back().transpose() << std::endl;

        // New approximation of matrix norm
        row_squared_norm = row.squaredNorm();
        row_norm         = sqrt(row_squared_norm);

        col_squared_norm = col.squaredNorm();
        col_norm         = sqrt(col_squared_norm);

        // Updating the matrix norm:
        matrix_norm += std::abs(gamma * gamma * row_squared_norm * col_squared_norm);

        for(int j = 0; j < computed_rank; j++) 
        {
            matrix_norm += 2.0 * std::abs(u[j].dot(u.back())) 
                               * std::abs(v[j].dot(v.back()));
        }
        
        computed_rank++;
    }
    while(computed_rank * (n_rows + n_cols) * row_norm * col_norm > 
          fabs(max) * tolerance * matrix_norm && 
          computed_rank < fmin(n_rows, n_cols)
         );

    // If the computed_rank is >= to full-rank
    // then return the trivial full-rank decomposition
    if (computed_rank >= fmin(n_rows, n_cols) - 1) 
    {
        if (n_rows < n_cols) 
        {
            L = Mat::Identity(n_rows, n_rows);
            R = this->A->getMatrix(n_row_start, n_col_start, n_rows, n_cols).transpose();
            computed_rank = n_rows;
        }

        else 
        {
            L = this->A->getMatrix(n_row_start, n_col_start, n_rows, n_cols);
            R = Mat::Identity(n_cols, n_cols);
            computed_rank = n_cols;
        }
    }
    
    // This is when ACA has succeeded:
    else 
    {
        L = Mat(n_rows, computed_rank);
        R = Mat(n_cols, computed_rank);
        
        for (int j = 0; j < computed_rank; j++) 
        {
            L.col(j) = u[j];
            R.col(j) = v[j];
        }
    }
}

void Matrix_Factorizer::SVD(Mat& L,  Mat& R, double tolerance_or_rank,
                            int n_row_start, int n_col_start, 
                            int n_rows, int n_cols
                           )
{
    Mat temp = this->A->getMatrix(n_row_start, n_col_start, n_rows, n_cols);
    Eigen::BDCSVD<Eigen::MatrixXd> svd(temp, Eigen::ComputeThinU | Eigen::ComputeThinV);

    svd.setThreshold(tolerance_or_rank);
    int rank = svd.rank();
    L        = svd.matrixU().block(0, 0, n_rows, rank) * svd.singularValues().block(0, 0, rank, 1).asDiagonal();
    R        = svd.matrixV().block(0, 0, n_cols, rank);
}

void Matrix_Factorizer::getFactorization(Mat& L,  Mat& R, double tolerance_or_rank,
                                         int n_row_start, int n_col_start, 
                                         int n_rows, int n_cols
                                        )
{
    if(n_rows == -1)
        n_rows = this->N;
    if(n_cols == -1)
        n_cols = this->N;

    if(this->type == "rookPivoting")
    {
        int computed_rank;
        rookPiv(L, R, tolerance_or_rank,
                n_row_start, n_col_start, 
                n_rows, n_cols
               );
    }

    else if(this->type == "queenPivoting")
    {
        int computed_rank;
        queenPiv(L, R, tolerance_or_rank,
                 n_row_start, n_col_start, 
                 n_rows, n_cols
                );
    }

    else if(this->type == "SVD")
    {
        SVD(L, R, tolerance_or_rank,
            n_row_start, n_col_start, 
            n_rows, n_cols
           );
    }

    else
    {
        std::cout << "Invalid option for matrix factorization" << std::endl;
        exit(1);
    }
}
