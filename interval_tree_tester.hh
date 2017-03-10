/*
 * interval_tree_tester.hh
 *
 *  Created on: Mar 10, 2017
 *      Author: simonm
 */

#ifndef INTERVAL_TREE_TESTER_HH_
#define INTERVAL_TREE_TESTER_HH_

#include "interval_tree.hh"
#include <unistd.h>
#include <iostream>
#include <iterator>

class interval_tree_tester
{
  public:

    void print()
    {
      print( tree.tree_root );
    }

    bool test_rb_invariant()
    {
      return test_rb_invariant( tree.tree_root ).first;
    }

    bool test_interval_invariant()
    {
      return test_interval_invariant( tree.tree_root );
    }

    void clear()
    {
      tree.clear();
    }

    void populate()
    {
      std::list< std::pair<int, int> > intervals;

      srand( time( NULL ) );

      for( int i = 0; i < 1000; ++i )
      {
        int m = rand() % 999 + 1;
        int l = rand() % m + 1;
        int h = m + rand() % ( 1000 - m ) + 1;
        std::stringstream ss;
        ss << "(" << l << ", " << h << ")";
        tree.insert( l, h, ss.str() );
        intervals.push_back( std::make_pair( l, h ) );
      }

      for( int i = 0; i < 200; ++i )
      {
        int index = rand() % intervals.size();
        auto itr = intervals.begin();
        std::advance( itr, index );
        tree.erase( itr->first, itr->second );
        intervals.erase( itr );
      }
    }

  private:

    void print( const std::unique_ptr< interval_node_t<int, std::string> > &root, const std::string &indent = "" )
    {
      if( !root ) return;

      print( root->right, indent + "  " );
      std::string colour = root->colour ? "(R)" : "(B)";
      std::cout << indent << root->value << colour << std::endl;
      print( root->left, indent + "  " );
    }

    static std::pair<bool, int> test_rb_invariant( const std::unique_ptr< interval_node_t<int, std::string> > &root )
    {
      // base case
      if( !root )
        return std::make_pair( true, 0 );

      int black = 0;
      if( root->colour == RED )
      {
        // RED node cannot have RED children
        if( ( root->left && root->left->colour == RED ) || ( root->right && root->right->colour == RED ) )
          return std::make_pair( false, -1 );
      }
      else
        black += 1;

      std::pair<bool, int> l = test_rb_invariant( root->left );
      std::pair<bool, int> r = test_rb_invariant( root->right );

      if( !l.first || !r.first )
        return std::make_pair( false, -1 );

      if( l.second != r.second )
        return std::make_pair( false, -1 );

      return std::make_pair( true, l.second + black );
    }

    static bool test_interval_invariant( const std::unique_ptr< interval_node_t<int, std::string> > &root )
    {
      // base case
      if( !root )
        return true;

      // max has to be >= high
      if( root->max < root->high )
        return false;

      // max has to be >= left->max
      if( root->left && root->max < root->left->max )
        return false;

      // max has to be >= right->max
      if( root->right && root->max < root->right->max )
        return false;

      // test children
      return test_interval_invariant( root->left ) && test_interval_invariant( root->right );
    }

    interval_tree<int, std::string> tree;
};



#endif /* INTERVAL_TREE_TESTER_HH_ */
