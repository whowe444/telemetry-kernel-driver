use crate::consumer::Consumer;
use crate::producer::Producer;

pub struct IpcToRabbit<C, P>
where C: Consumer,
    P: Producer
{
    consumer: C,
    producer: P
}

impl<C, P> IpcToRabbit<C, P>
where
    C: Consumer,
    P: Producer
{
    pub fn new(consumer: C, producer: P) -> Self {
        Self {consumer, producer}
    }

    pub async fn start(&self) {
        loop {
            self.process().await;
        }
    }

    pub async fn process(&self) {
        match self.consumer.accept(self.producer.get()).await {
            Ok(_) => println!("Message processed successfully!"),
            Err(e) => eprintln!("Error processing message: {}!", e)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use crate::consumer::MockConsumer;
    use crate::producer::MockProducer;

    #[tokio::test]
    async fn test_process() {
        // Create Consumer
        let mut consumer = MockConsumer::new();

        // Create Producer
        let mut producer = MockProducer::new();

        producer
            .expect_get()
            .times(1..)
            .returning(|| vec![1, 2, 3]);

        consumer
            .expect_accept()
            .times(1..)
            .returning(|_| Ok(()));

        // Create unit under test
        let processor = IpcToRabbit::new(consumer, producer);

        // Call unit under test
        processor.process().await;
    }
}