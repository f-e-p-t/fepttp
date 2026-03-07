import { useState } from "react";

import ListGroup from "./components/ListGroup.tsx";
import Alert from "./components/Alert.tsx";
import BSButton from "./components/BSButton.tsx";
import MovieList from "./components/MovieList.tsx";

import './styles/MovieList.css';

function  App(){
  const [alertVisible, setAlertVisible] = useState(false);

  const items1 = ['Item one', 'Item two', 'Item three', 'Item four'];
  const items2 = ['First item', 'Second item', 'Third item', 'Fourth item'];

  const handleSelectItem = (item: string) => {
    console.log(item);
  }

  return(
    <>
      <div>
        {/* pass in  each attribute of the Props interface */}
        <ListGroup items={items1} heading={'List of items'} onSelectItem={handleSelectItem}/>
        <ListGroup items={items2} heading={'Another list of items'} onSelectItem={handleSelectItem}/>
      </div>

      <div>
        <Alert dismissible={false} onClose={() => {}}>
          <span>This is an alert-success</span>
        </Alert>
      </div>

      <div>
        <BSButton colour='danger' onClick={() => {console.log('Danger button clicked!')}}>
          Danger Button
        </BSButton>
      </div>
      <div><p></p></div>

      <div>
        {alertVisible && <Alert dismissible={true} onClose={() => {
          setAlertVisible(false);}
        }
        ><b>This is an alert!</b> Dismiss --{'>'}</Alert>}
        <BSButton colour='primary' onClick={() => {setAlertVisible(true);}}>
          Alert Button
        </BSButton>
      </div>

      <div>
        <MovieList/>
      </div>

    </>
  );
}

export default App;