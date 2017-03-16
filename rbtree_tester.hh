/*
 * rbtree_tester.hh
 *
 *  Created on: Mar 9, 2017
 *      Author: simonm
 */

#ifndef RBTREE_TESTER_HH_
#define RBTREE_TESTER_HH_

#include "rbtree.hh"
#include <unistd.h>
#include <iostream>

class rbtree_tester
{
  public:

    void print()
    {
      print( tree.tree_root );
    }

    bool test_invariant()
    {
      return test_invariant( tree.tree_root ).first;
    }

    bool test_iterator()
    {
      tree.clear();

      tree.insert( 1, "1" );
      tree.insert( 2, "2" );
      tree.insert( 3, "3" );
      tree.insert( 4, "4" );
      tree.insert( 5, "5" );
      tree.insert( 6, "6" );
      tree.insert( 7, "7" );
      tree.insert( 8, "8" );
      tree.insert( 9, "9" );

      int i = 1;
      rbtree<int, std::string>::iterator itr;
      for( itr = tree.begin() ; itr != tree.end(); ++itr )
      {
        if( itr->key != i )
          return false;
        ++i;
      }

      return true;
    }

    void clear()
    {
      tree.clear();
    }

    void populate()
    {
      srand( time( NULL ) );

      for( int i = 0; i < 1000; ++i )
      {
        int k = rand() % 1000 + 1;
        std::stringstream ss;
        ss << k;
        tree.insert( k, ss.str() );
      }

      for( int i = 0; i < 200; ++i )
      {
        int k = rand() % 1000 + 1;
        tree.erase( k );
      }
    }

  private:

    void print( const std::unique_ptr< node_t<int, std::string> > &root, const std::string &indent = "" )
    {
      if( !root ) return;

      print( root->right, indent + "  " );
      std::string colour = root->colour ? "(R)" : "(B)";
      std::cout << indent << root->key << colour << std::endl;
      print( root->left, indent + "  " );
    }

    static std::pair<bool, int> test_invariant( const std::unique_ptr< node_t<int, std::string> > &root )
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

      std::pair<bool, int> l = test_invariant( root->left );
      std::pair<bool, int> r = test_invariant( root->right );

      if( !l.first || !r.first )
        return std::make_pair( false, -1 );

      if( l.second != r.second )
        return std::make_pair( false, -1 );

      return std::make_pair( true, l.second + black );
    }

    rbtree<int, std::string> tree;
};

#endif /* RBTREE_TESTER_HH_ */
